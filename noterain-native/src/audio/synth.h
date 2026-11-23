// NoteRain Native — built-in piano synthesiser
#pragma once

#include <atomic>
#include <cstdint>

#include "core/spsc.h"

namespace nr {

struct SynthEvent {
  enum Type : uint8_t { NoteOn = 0, NoteOff, PedalOn, PedalOff, AllOff, Click };
  Type type = NoteOn;
  int key = 60;
  int velocity = 80;
  int accent = 0;
};

/**
 * Additive piano-ish synthesiser.
 *
 * Six slightly inharmonic partials per voice with per-partial decay, a short
 * attack ramp, a damper on note-off and full sustain-pedal behaviour. Rendering
 * happens on the audio thread; all note events arrive through a lock-free queue.
 */
class PianoSynth {
public:
  static constexpr int kMaxVoices = 96;
  static constexpr int kPartials = 6;

  void prepare(double sampleRate);

  void queueNoteOn(int key, int velocity);
  void queueNoteOff(int key);
  void queuePedal(bool down);
  void queueAllOff();
  void queueClick(bool accent);

  void setMasterGain(float gain) { master_ = gain; }
  float masterGain() const { return master_; }
  void setMetronomeGain(float gain) { metronomeGain_ = gain; }

  /** Render interleaved stereo float samples. */
  void render(float* interleaved, int frames);

  /** Rough output level in [0,1] for meters. */
  float lastPeak() const { return peak_; }

private:
  struct Partial {
    double phase = 0.0;
    double inc = 0.0;
    double env = 0.0;
    double envDecay = 0.9999;
    double amp = 0.0;
  };

  struct Voice {
    bool active = false;
    bool released = false;
    bool sustained = false;
    int key = -1;
    double attack = 0.0;      // 0..1 ramp
    double attackInc = 0.0;
    double release = 1.0;     // multiplier while released
    double releaseDecay = 1.0;
    float panL = 0.707f;
    float panR = 0.707f;
    int partials = 0;
    Partial p[kPartials];
    double age = 0.0;
  };

  struct Click {
    bool active = false;
    double phase = 0.0;
    double inc = 0.0;
    double env = 0.0;
    double decay = 0.999;
    float gain = 0.4f;
  };

  void handle(const SynthEvent& ev);
  void startVoice(int key, int velocity);
  void releaseKey(int key);
  void releaseAllSustained();
  int allocateVoice();

  SpscQueue<SynthEvent, 512> queue_;
  Voice voices_[kMaxVoices];
  Click clicks_[8];
  double sr_ = 48000.0;
  float master_ = 0.8f;
  float metronomeGain_ = 0.5f;
  bool pedal_ = false;
  float peak_ = 0.0f;
};

} // namespace nr
