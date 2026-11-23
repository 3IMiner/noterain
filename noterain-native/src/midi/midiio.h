// NoteRain Native — Windows MIDI input / output (winmm)
#pragma once

#include <string>
#include <vector>

#include <windows.h>
#include <mmsystem.h>

#include "core/spsc.h"

namespace nr {

struct MidiMessage {
  int status = 0;
  int data1 = 0;
  int data2 = 0;
};

/** Hardware MIDI keyboard input via winmm (callback driven, ~1-3 ms latency). */
class MidiInput {
public:
  MidiInput() = default;
  ~MidiInput();

  static std::vector<std::string> deviceNames();

  bool open(int deviceIndex);
  void close();
  bool isOpen() const { return handle_ != nullptr; }
  int deviceIndex() const { return index_; }
  const std::string& error() const { return error_; }
  std::string deviceName() const;

  /** Drain one queued message (channel messages only). */
  bool poll(MidiMessage& out);

private:
  static void CALLBACK midiCallback(HMIDIIN handle, UINT msg, DWORD_PTR user,
                                    DWORD_PTR param1, DWORD_PTR param2);
  void onShortMessage(DWORD packed);

  HMIDIIN handle_ = nullptr;
  int index_ = -1;
  std::string error_;
  SpscQueue<MidiMessage, 512> queue_;
};

/** Optional MIDI output used to drive an external instrument while playing. */
class MidiOutput {
public:
  MidiOutput() = default;
  ~MidiOutput();

  static std::vector<std::string> deviceNames();

  bool open(int deviceIndex);
  void close();
  bool isOpen() const { return handle_ != nullptr; }
  int deviceIndex() const { return index_; }
  void send(int status, int data1, int data2);
  void allNotesOff();

private:
  HMIDIOUT handle_ = nullptr;
  int index_ = -1;
};

} // namespace nr
