// NoteRain Native — audio output
#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "audio/synth.h"

namespace nr {

/**
 * WASAPI shared-mode output driving a PianoSynth on its own thread.
 *
 * The stream is initialised as 48 kHz / stereo / float32 and WASAPI converts to
 * whatever the device mix format is (AUTOCONVERTPCM), so any sound card works.
 */
class AudioEngine {
public:
  AudioEngine() = default;
  ~AudioEngine();

  bool start(PianoSynth* synth);
  void stop();
  bool running() const { return running_; }
  const std::string& error() const { return error_; }
  double sampleRate() const { return sampleRate_; }
  float latencyMs() const { return latencyMs_; }

  /** Friendly names of the active render endpoints. */
  static std::vector<std::string> outputDevices();

private:
  void runLoop();

  PianoSynth* synth_ = nullptr;
  std::thread thread_;
  std::atomic<bool> stop_{false};
  std::atomic<bool> initDone_{false};
  std::atomic<bool> initOk_{false};
  std::string error_;
  double sampleRate_ = 48000.0;
  float latencyMs_ = 0.0f;
  bool running_ = false;
};

} // namespace nr
