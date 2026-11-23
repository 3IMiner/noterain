#include "audio/synth.h"

#include <algorithm>
#include <cmath>

namespace nr {
namespace {

constexpr int kTableSize = 4096;
constexpr int kTableMask = kTableSize - 1;

float* sineTable() {
  static float table[kTableSize + 1];
  static bool ready = false;
  if (!ready) {
    for (int i = 0; i <= kTableSize; ++i) {
      table[i] = (float)std::sin((double)i / (double)kTableSize * 6.283185307179586);
    }
    ready = true;
  }
  return table;
}

inline float tableLookup(const float* table, double phase) {
  const double wrapped = phase - std::floor(phase / (double)kTableSize) * (double)kTableSize;
  const int index = (int)wrapped;
  const float frac = (float)(wrapped - index);
  const float a = table[index];
  const float b = table[index + 1];
  return a + (b - a) * frac;
}

inline double midiToHz(int key) { return 440.0 * std::pow(2.0, (key - 69) / 12.0); }

inline float softClip(float x) {
  if (x > 1.5f) return 1.0f;
  if (x < -1.5f) return -1.0f;
  return x - (x * x * x) / 6.75f;
}

} // namespace

void PianoSynth::prepare(double sampleRate) {
  sr_ = sampleRate > 8000.0 ? sampleRate : 48000.0;
  for (auto& v : voices_) v.active = false;
  for (auto& c : clicks_) c.active = false;
  pedal_ = false;
}

int PianoSynth::allocateVoice() {
  int oldestReleased = -1;
  int oldest = 0;
  double oldestAge = -1.0;
  for (int i = 0; i < kMaxVoices; ++i) {
    if (!voices_[i].active) return i;
    if (voices_[i].released) {
      if (oldestReleased < 0 || voices_[i].age > voices_[oldestReleased].age)
        oldestReleased = i;
    }
    if (voices_[i].age > oldestAge) {
      oldestAge = voices_[i].age;
      oldest = i;
    }
  }
  return oldestReleased >= 0 ? oldestReleased : oldest;
}

void PianoSynth::startVoice(int key, int velocity) {
  Voice& v = voices_[allocateVoice()];
  v = Voice{};
  v.active = true;
  v.key = key;
  v.age = 0.0;
  v.attack = 0.0;
  v.attackInc = 1.0 / std::max(1.0, sr_ * 0.0045); // ~4.5 ms attack
  v.release = 1.0;
  v.releaseDecay = 1.0;

  const double vel = std::clamp(velocity / 127.0, 0.02, 1.0);
  const double f0 = midiToHz(key);
  const double inharmonicity = 0.00028;

  // Slight stereo spread across the keyboard.
  const double pan = std::clamp((key - 60) / 45.0, -1.0, 1.0) * 0.55;
  v.panL = (float)std::sqrt(0.5 * (1.0 - pan));
  v.panR = (float)std::sqrt(0.5 * (1.0 + pan));

  v.partials = kPartials;
  for (int i = 0; i < kPartials; ++i) {
    const double n = (double)(i + 1);
    const double freq = n * f0 * std::sqrt(1.0 + inharmonicity * n * n);
    if (freq > sr_ * 0.45) {
      v.partials = i;
      break;
    }
    Partial& p = v.p[i];
    p.phase = 0.0;
    p.inc = freq * (double)kTableSize / sr_;
    // Upper partials are quieter and vanish faster; harder hits are brighter.
    double brightness = (i == 0) ? 1.0 : std::pow(1.0 / n, 1.25) * (0.18 + 0.82 * vel * vel);
    double tau = (2.9 / std::pow(n, 0.72)) * (0.72 + 0.55 * vel);
    if (i == 0) tau *= 1.25;
    p.amp = brightness * (0.55 + 0.45 * vel);
    p.env = 1.0;
    p.envDecay = std::exp(-1.0 / (tau * sr_));
  }
}

void PianoSynth::releaseKey(int key) {
  for (auto& v : voices_) {
    if (!v.active || v.key != key || v.released) continue;
    if (pedal_) {
      v.sustained = true;
      continue;
    }
    v.released = true;
    v.releaseDecay = std::exp(-1.0 / (0.13 * sr_));
  }
}

void PianoSynth::releaseAllSustained() {
  for (auto& v : voices_) {
    if (v.active && v.sustained && !v.released) {
      v.sustained = false;
      v.released = true;
      v.releaseDecay = std::exp(-1.0 / (0.16 * sr_));
    }
  }
}

void PianoSynth::handle(const SynthEvent& ev) {
  switch (ev.type) {
    case SynthEvent::NoteOn:
      if (ev.velocity > 0) startVoice(ev.key, ev.velocity);
      else releaseKey(ev.key);
      break;
    case SynthEvent::NoteOff:
      releaseKey(ev.key);
      break;
    case SynthEvent::PedalOn:
      pedal_ = true;
      break;
    case SynthEvent::PedalOff:
      pedal_ = false;
      releaseAllSustained();
      break;
    case SynthEvent::AllOff:
      for (auto& v : voices_) v.active = false;
      pedal_ = false;
      break;
    case SynthEvent::Click: {
      for (auto& c : clicks_) {
        if (c.active) continue;
        c.active = true;
        c.phase = 0.0;
        c.inc = (ev.accent ? 1600.0 : 1050.0) * (double)kTableSize / sr_;
        c.env = 1.0;
        c.decay = std::exp(-1.0 / (0.028 * sr_));
        c.gain = ev.accent ? 0.55f : 0.35f;
        break;
      }
      break;
    }
  }
}

void PianoSynth::queueNoteOn(int key, int velocity) {
  queue_.push({SynthEvent::NoteOn, key, velocity, 0});
}
void PianoSynth::queueNoteOff(int key) {
  queue_.push({SynthEvent::NoteOff, key, 0, 0});
}
void PianoSynth::queuePedal(bool down) {
  queue_.push({down ? SynthEvent::PedalOn : SynthEvent::PedalOff, 0, 0, 0});
}
void PianoSynth::queueAllOff() { queue_.push({SynthEvent::AllOff, 0, 0, 0}); }
void PianoSynth::queueClick(bool accent) {
  queue_.push({SynthEvent::Click, 0, 0, accent ? 1 : 0});
}

void PianoSynth::render(float* out, int frames) {
  SynthEvent ev;
  while (queue_.pop(ev)) handle(ev);

  const float* table = sineTable();
  const float master = master_;

  for (int i = 0; i < frames; ++i) {
    float left = 0.0f;
    float right = 0.0f;

    for (auto& v : voices_) {
      if (!v.active) continue;

      if (v.attack < 1.0) {
        v.attack = std::min(1.0, v.attack + v.attackInc);
      }
      if (v.released) {
        v.release *= v.releaseDecay;
        if (v.release < 0.0004) {
          v.active = false;
          continue;
        }
      }

      float sample = 0.0f;
      bool anyAlive = false;
      for (int k = 0; k < v.partials; ++k) {
        Partial& p = v.p[k];
        p.env *= p.envDecay;
        if (p.env > 0.00025) anyAlive = true;
        sample += tableLookup(table, p.phase) * (float)(p.env * p.amp);
        p.phase += p.inc;
      }
      if (!anyAlive) {
        v.active = false;
        continue;
      }

      const float amp = (float)v.attack * (float)v.release;
      left += sample * amp * v.panL;
      right += sample * amp * v.panR;
      v.age += 1.0 / sr_;
    }

    for (auto& c : clicks_) {
      if (!c.active) continue;
      c.env *= c.decay;
      if (c.env < 0.0005) {
        c.active = false;
        continue;
      }
      const float s = tableLookup(table, c.phase) * (float)c.env * c.gain * metronomeGain_;
      left += s;
      right += s;
      c.phase += c.inc;
    }

    left = softClip(left * master * 0.9f);
    right = softClip(right * master * 0.9f);
    out[i * 2 + 0] = left;
    out[i * 2 + 1] = right;

    const float peak = std::max(std::fabs(left), std::fabs(right));
    if (peak > peak_) peak_ = peak;
  }

  peak_ *= 0.94f;
}

} // namespace nr
