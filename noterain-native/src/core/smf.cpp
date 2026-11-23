#include "core/smf.h"

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>

namespace nr {
namespace {

struct ChannelEvent {
  long tick = 0;
  int status = 0; // full status byte (includes channel in low nibble)
  int d1 = 0;
  int d2 = 0;
};

struct ParsedTrack {
  std::string name;
  std::vector<ChannelEvent> events;
};

struct TempoMark {
  long tick = 0;
  double usPerQuarter = 500000.0;
};

struct TempoSegment {
  long tick = 0;
  double usPerQuarter = 500000.0;
  double time = 0.0;
};

uint32_t readU32(const uint8_t* p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

uint16_t readU16(const uint8_t* p) { return (uint16_t)((p[0] << 8) | p[1]); }

std::string utf8Of(const std::filesystem::path& path) {
  const std::wstring wide = path.wstring();
  if (wide.empty()) return {};
  const int size =
      WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
  if (size <= 0) return {};
  std::string out((size_t)size, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), out.data(), size, nullptr, nullptr);
  return out;
}

bool readVarLen(const uint8_t*& p, const uint8_t* end, uint32_t& value) {
  value = 0;
  int guard = 0;
  while (p < end && guard++ < 4) {
    const uint8_t byte = *p++;
    value = (value << 7) | (byte & 0x7F);
    if ((byte & 0x80) == 0) return true;
  }
  return false;
}

} // namespace

bool loadMidiFile(const std::filesystem::path& path, Song& out, std::string& error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    error = "cannot open file";
    return false;
  }
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
  if (data.size() < 14) {
    error = "file too small / not a MIDI file";
    return false;
  }
  if (std::memcmp(data.data(), "MThd", 4) != 0) {
    error = "missing MThd header";
    return false;
  }

  const uint16_t format = readU16(&data[8]);
  const uint16_t declaredTracks = readU16(&data[10]);
  const uint16_t division = readU16(&data[12]);
  if (division & 0x8000) {
    error = "SMPTE time division is not supported";
    return false;
  }
  const int ppq = division > 0 ? division : 480;

  Song song;
  song.ticksPerQuarter = ppq;
  song.title = path.stem().string();
  song.path = utf8Of(path);

  std::vector<ParsedTrack> parsed;
  std::vector<TempoMark> tempoMarks;
  std::vector<TimeSigPoint> timeSigs; // tick stored temporarily in `time`
  size_t pos = 14;
  int guardTracks = 0;

  while (pos + 8 <= data.size() && guardTracks++ < 512) {
    const uint8_t* chunkId = &data[pos];
    const uint32_t chunkLen = readU32(&data[pos + 4]);
    const size_t chunkStart = pos + 8;
    if (chunkStart + chunkLen > data.size()) break;

    if (std::memcmp(chunkId, "MTrk", 4) == 0) {
      ParsedTrack track;
      const uint8_t* p = &data[chunkStart];
      const uint8_t* end = p + chunkLen;
      long tick = 0;
      int running = 0;

      while (p < end) {
        uint32_t delta = 0;
        if (!readVarLen(p, end, delta)) break;
        tick += (long)delta;
        if (p >= end) break;

        int status = *p;
        if (status & 0x80) {
          ++p;
          running = status;
        } else {
          status = running;
          if (status == 0) break;
        }

        if (status == 0xFF) {
          if (p >= end) break;
          const int type = *p++;
          uint32_t len = 0;
          if (!readVarLen(p, end, len)) break;
          if (p + len > end) break;
          if (type == 0x51 && len >= 3) {
            const uint32_t us = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
            tempoMarks.push_back({tick, (double)us});
          } else if (type == 0x58 && len >= 2) {
            TimeSigPoint ts;
            ts.time = (double)tick;
            ts.num = p[0];
            ts.den = 1 << p[1];
            if (ts.num < 1) ts.num = 4;
            if (ts.den < 1) ts.den = 4;
            timeSigs.push_back(ts);
          } else if (type == 0x03 && len > 0) {
            track.name.assign((const char*)p, (size_t)len);
            // Trim trailing NULs / whitespace.
            while (!track.name.empty() &&
                   (track.name.back() == '\0' || track.name.back() == ' '))
              track.name.pop_back();
          } else if (type == 0x2F) {
            p = end; // end of track
            break;
          }
          p += len;
        } else if (status == 0xF0 || status == 0xF7) {
          uint32_t len = 0;
          if (!readVarLen(p, end, len)) break;
          p += len;
          if (p > end) break;
        } else {
          const int kind = status & 0xF0;
          const int d1 = (p < end) ? *p++ : 0;
          const int d2 = (kind == 0x80 || kind == 0x90 || kind == 0xA0 || kind == 0xB0 ||
                          kind == 0xE0)
                             ? ((p < end) ? *p++ : 0)
                             : 0;
          if (kind == 0x80 || kind == 0x90) {
            ChannelEvent ev;
            ev.tick = tick;
            ev.status = status;
            ev.d1 = d1;
            ev.d2 = d2;
            track.events.push_back(ev);
          } else if (kind == 0xC0) {
            ChannelEvent ev;
            ev.tick = tick;
            ev.status = status;
            ev.d1 = d1;
            ev.d2 = 0;
            track.events.push_back(ev);
          }
        }
      }
      parsed.push_back(std::move(track));
    }
    pos = chunkStart + chunkLen;
    if (pos & 1) { /* chunks are byte aligned */ }
  }

  if (parsed.empty()) {
    error = "no MIDI tracks found";
    return false;
  }
  (void)format;
  (void)declaredTracks;

  // --- Build the tempo map (ticks -> seconds) -----------------------------
  std::sort(tempoMarks.begin(), tempoMarks.end(),
            [](const TempoMark& a, const TempoMark& b) { return a.tick < b.tick; });
  if (tempoMarks.empty() || tempoMarks[0].tick != 0) tempoMarks.insert(tempoMarks.begin(), {0, 500000.0});

  std::vector<TempoSegment> segments;
  segments.push_back({0, tempoMarks[0].usPerQuarter, 0.0});
  for (size_t i = 1; i < tempoMarks.size(); ++i) {
    const TempoSegment& prev = segments.back();
    const double quarterNotes = (double)(tempoMarks[i].tick - prev.tick) / (double)ppq;
    const double time = prev.time + quarterNotes * prev.usPerQuarter / 1.0e6;
    if (tempoMarks[i].usPerQuarter <= 0.0) continue;
    segments.push_back({tempoMarks[i].tick, tempoMarks[i].usPerQuarter, time});
  }
  // Times in seconds for tempo points (for the Song's tempo map).
  song.tempos.clear();
  for (const auto& seg : segments) {
    TempoPoint tp;
    tp.time = seg.time;
    tp.bpm = 60.0e6 / seg.usPerQuarter;
    song.tempos.push_back(tp);
  }
  if (!song.tempos.empty()) song.tempos[0].time = 0.0;

  auto tickToTime = [&](long tick) -> double {
    size_t lo = 0;
    for (size_t i = 0; i < segments.size(); ++i) {
      if (segments[i].tick <= tick) lo = i;
      else break;
    }
    const TempoSegment& seg = segments[lo];
    const double quarters = (double)(tick - seg.tick) / (double)ppq;
    return seg.time + quarters * seg.usPerQuarter / 1.0e6;
  };

  // Time signature map.
  song.timeSigs.clear();
  for (auto& ts : timeSigs) {
    const double t = tickToTime((long)ts.time);
    song.timeSigs.push_back({t, ts.num, ts.den});
  }

  // --- Convert channel events to notes ------------------------------------
  struct Pending {
    double start = 0.0;
    int velocity = 80;
    int noteIndex = -1;
  };

  for (auto& pt : parsed) {
    Track track;
    track.name = pt.name;
    track.channel = -1;

    std::map<int, std::vector<Pending>> pending; // key: channel<<8 | key

    for (const auto& ev : pt.events) {
      const int kind = ev.status & 0xF0;
      const int channel = ev.status & 0x0F;
      if (kind == 0xC0) {
        track.program = ev.d1;
        if (track.channel < 0) track.channel = channel;
        continue;
      }
      const int key = ev.d1;
      const int velocity = ev.d2;
      const int id = (channel << 8) | key;

      if (kind == 0x90 && velocity > 0) {
        if (track.channel < 0) track.channel = channel;
        Note note;
        note.channel = channel;
        note.key = key;
        note.velocity = velocity;
        note.start = tickToTime(ev.tick);
        note.dur = 0.25;
        track.notes.push_back(note);
        pending[id].push_back({note.start, velocity, (int)track.notes.size() - 1});
      } else if (kind == 0x80 || (kind == 0x90 && velocity == 0)) {
        auto it = pending.find(id);
        if (it != pending.end() && !it->second.empty()) {
          Pending p = it->second.front();
          it->second.erase(it->second.begin());
          if (p.noteIndex >= 0 && p.noteIndex < (int)track.notes.size()) {
            double end = tickToTime(ev.tick);
            double dur = end - p.start;
            if (dur < 0.03) dur = 0.03;
            track.notes[(size_t)p.noteIndex].dur = dur;
          }
        }
      }
    }

    // Close dangling notes with a sensible default length.
    for (auto& [id, list] : pending) {
      (void)id;
      for (const auto& p : list) {
        if (p.noteIndex >= 0 && p.noteIndex < (int)track.notes.size())
          track.notes[(size_t)p.noteIndex].dur = 0.4;
      }
    }

    if (track.name.empty()) {
      track.name = pt.name.empty() ? ("Track " + std::to_string(song.tracks.size() + 1))
                                   : pt.name;
    }
    if (track.channel < 0) track.channel = 0;
    song.tracks.push_back(std::move(track));
  }

  song.finalize();
  if (song.empty()) {
    error = "MIDI file contains no notes";
    return false;
  }
  out = std::move(song);
  return true;
}

// ---------------------------------------------------------------------------
// Built-in demo song — a short two-hand study used on first launch.
// ---------------------------------------------------------------------------
Song makeDemoSong() {
  Song song;
  song.title = "C 大调练习曲 (Demo)";
  song.ticksPerQuarter = 480;
  song.tempos = {{0.0, 100.0, 0.0}};
  song.timeSigs = {{0.0, 4, 4}};

  Track right;
  right.name = "右手 Melody";
  right.channel = 0;
  right.color = 0;

  Track left;
  left.name = "左手 Bass";
  left.channel = 1;
  left.color = 1;

  const double beat = 60.0 / 100.0; // 0.6 s
  auto add = [&](Track& tr, int key, double startBeat, double beats, int vel) {
    Note n;
    n.key = key;
    n.velocity = vel;
    n.start = startBeat * beat;
    n.dur = beats * beat * 0.92;
    n.channel = tr.channel;
    tr.notes.push_back(n);
  };

  // Melody (treble): 8 bars of quarter notes, I–vi–IV–V style progression.
  const int melody[] = {72, 76, 79, 76, 72, 74, 76, 74, 77, 76, 74, 72, 71, 72, 74, 71,
                        72, 76, 79, 81, 79, 77, 76, 74, 72, 71, 69, 67, 72, 76, 72, 72};
  const int bassRoots[] = {48, 45, 41, 43, 48, 45, 41, 43}; // C3 A2 F2 G2

  for (int bar = 0; bar < 8; ++bar) {
    for (int q = 0; q < 4; ++q) {
      const int idx = bar * 4 + q;
      add(right, melody[idx], (double)(bar * 4 + q), 1.0, q == 0 ? 96 : 78);
    }
    const int root = bassRoots[bar];
    // Bass: root on beat 1 & 3, chord tones in between.
    add(left, root, (double)(bar * 4 + 0), 1.9, 70);
    add(left, root + 7, (double)(bar * 4 + 0), 1.9, 62);
    add(left, root + 12, (double)(bar * 4 + 2), 1.9, 66);
    add(left, root + 7, (double)(bar * 4 + 2), 1.9, 58);
  }

  song.tracks.push_back(std::move(right));
  song.tracks.push_back(std::move(left));
  song.finalize();
  return song;
}

} // namespace nr
