// NoteRain Native — song model
#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace nr {

struct Note {
  int track = 0;
  int channel = 0;
  int key = 60;      // MIDI note number
  int velocity = 80; // 1..127
  double start = 0;  // seconds
  double dur = 0.5;  // seconds

  double end() const { return start + dur; }
  bool leftHand() const { return key < 60; }
};

struct Track {
  std::string name;
  int channel = 0;
  int program = 0;
  int color = 0;
  bool enabled = true;
  std::vector<Note> notes;

  double avgKey = 60.0;
  int minKey = 127;
  int maxKey = 0;
};

struct TempoPoint {
  double time = 0.0;
  double bpm = 120.0;
  double beat = 0.0; // musical quarter-note position of `time`
};

struct TimeSigPoint {
  double time = 0.0;
  int num = 4;
  int den = 4;
};

/**
 * A parsed MIDI song.
 *
 * `ordered` holds pointers to every note sorted by start time; measure maths is
 * precomputed in `measureTimes` so the UI can jump bars cheaply.
 */
struct Song {
  std::string title;
  std::string path;
  int ticksPerQuarter = 480;
  double duration = 0.0;
  int minKey = 21;
  int maxKey = 108;

  std::vector<Track> tracks;
  std::vector<TempoPoint> tempos;
  std::vector<TimeSigPoint> timeSigs;
  std::vector<const Note*> ordered;
  std::vector<double> measureTimes; // start time of each measure

  bool empty() const { return ordered.empty(); }
  int noteCount() const { return (int)ordered.size(); }

  void clear() {
    title.clear();
    path.clear();
    tracks.clear();
    tempos.clear();
    timeSigs.clear();
    ordered.clear();
    measureTimes.clear();
    duration = 0.0;
    minKey = 21;
    maxKey = 108;
    ticksPerQuarter = 480;
  }

  const TempoPoint& tempoAt(double t) const {
    if (tempos.empty()) {
      static const TempoPoint fallback{0.0, 120.0, 0.0};
      return fallback;
    }
    size_t lo = 0;
    for (size_t i = 0; i < tempos.size(); ++i) {
      if (tempos[i].time <= t + 1e-9) lo = i;
      else break;
    }
    return tempos[lo];
  }

  double bpmAt(double t) const { return tempoAt(t).bpm; }

  /** Musical quarter-note position at time t (tempo-aware). */
  double beatAt(double t) const {
    const TempoPoint& tp = tempoAt(t);
    return tp.beat + (t - tp.time) * tp.bpm / 60.0;
  }

  /** Time of a musical quarter-note position (tempo-aware). */
  double timeAtBeat(double beat) const {
    if (tempos.empty()) return beat * 0.5;
    const TempoPoint* cur = &tempos[0];
    for (const auto& tp : tempos) {
      if (tp.beat <= beat + 1e-9) cur = &tp;
      else break;
    }
    return cur->time + (beat - cur->beat) * 60.0 / cur->bpm;
  }

  const TimeSigPoint& timeSigAt(double t) const {
    if (timeSigs.empty()) {
      static const TimeSigPoint fallback{0.0, 4, 4};
      return fallback;
    }
    const TimeSigPoint* cur = &timeSigs[0];
    for (const auto& ts : timeSigs) {
      if (ts.time <= t + 1e-9) cur = &ts;
      else break;
    }
    return *cur;
  }

  /** Length of the measure starting at time t, in seconds. */
  double measureSeconds(double t) const {
    const TimeSigPoint& ts = timeSigAt(t);
    const double quarters = ts.num * 4.0 / std::max(1, ts.den);
    return quarters * 60.0 / std::max(20.0, bpmAt(t));
  }

  int measureCount() const { return (int)std::max<size_t>(1, measureTimes.size()); }

  double measureStart(int measure) const {
    if (measureTimes.empty()) return 0.0;
    if (measure < 0) measure = 0;
    if (measure >= (int)measureTimes.size()) return duration;
    return measureTimes[measure];
  }

  int measureAt(double t) const {
    if (measureTimes.empty()) return 0;
    int lo = 0;
    for (int i = 0; i < (int)measureTimes.size(); ++i) {
      if (measureTimes[i] <= t + 1e-6) lo = i;
      else break;
    }
    return lo;
  }

  /** Rebuild derived data after notes are added/edited. */
  void finalize() {
    // Drop empty tracks, compute per-track statistics.
    std::vector<Track> kept;
    kept.reserve(tracks.size());
    for (auto& tr : tracks) {
      if (tr.notes.empty()) continue;
      tr.minKey = 127;
      tr.maxKey = 0;
      double sum = 0.0;
      for (auto& n : tr.notes) {
        n.track = (int)kept.size();
        tr.minKey = std::min(tr.minKey, n.key);
        tr.maxKey = std::max(tr.maxKey, n.key);
        sum += n.key;
        if (n.dur < 0.05) n.dur = 0.05;
      }
      tr.avgKey = sum / (double)tr.notes.size();
      kept.push_back(std::move(tr));
    }
    tracks = std::move(kept);

    if (tracks.empty()) {
      clear();
      return;
    }

    // Sort tempo / time-signature maps.
    if (tempos.empty()) tempos.push_back({0.0, 120.0, 0.0});
    std::sort(tempos.begin(), tempos.end(),
              [](const TempoPoint& a, const TempoPoint& b) { return a.time < b.time; });
    // Recompute musical position of every tempo point.
    double beat = 0.0;
    for (size_t i = 0; i < tempos.size(); ++i) {
      if (i > 0) {
        const auto& prev = tempos[i - 1];
        beat = prev.beat + (tempos[i].time - prev.time) * prev.bpm / 60.0;
      } else {
        beat = 0.0;
      }
      tempos[i].beat = beat;
    }
    if (timeSigs.empty()) timeSigs.push_back({0.0, 4, 4});
    std::sort(timeSigs.begin(), timeSigs.end(),
              [](const TimeSigPoint& a, const TimeSigPoint& b) { return a.time < b.time; });

    // Flatten notes.
    ordered.clear();
    ordered.reserve(1024);
    minKey = 127;
    maxKey = 0;
    double endTime = 0.0;
    for (auto& tr : tracks) {
      for (auto& n : tr.notes) {
        ordered.push_back(&n);
        minKey = std::min(minKey, n.key);
        maxKey = std::max(maxKey, n.key);
        endTime = std::max(endTime, n.end());
      }
    }
    std::sort(ordered.begin(), ordered.end(), [](const Note* a, const Note* b) {
      if (a->start != b->start) return a->start < b->start;
      return a->key < b->key;
    });
    duration = endTime + 0.5;
    minKey = std::max(21, minKey);
    maxKey = std::min(108, maxKey);

    // Precompute measure start times (walk the tempo/time-signature map).
    measureTimes.clear();
    measureTimes.push_back(0.0);
    double t = 0.0;
    int guard = 0;
    while (t < duration + 1e-6 && guard < 20000) {
      t += measureSeconds(t);
      measureTimes.push_back(t);
      ++guard;
    }
  }
};

} // namespace nr
