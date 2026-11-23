#include "midi/midiio.h"

#include <algorithm>

namespace nr {
namespace {

std::string toUtf8(const wchar_t* text) {
  if (!text || text[0] == L'\0') return {};
  const int len = (int)wcslen(text);
  const int size = WideCharToMultiByte(CP_UTF8, 0, text, len, nullptr, 0, nullptr, nullptr);
  if (size <= 0) return {};
  std::string out((size_t)size, '\0');
  WideCharToMultiByte(CP_UTF8, 0, text, len, out.data(), size, nullptr, nullptr);
  return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

MidiInput::~MidiInput() { close(); }

std::vector<std::string> MidiInput::deviceNames() {
  std::vector<std::string> names;
  const UINT count = midiInGetNumDevs();
  for (UINT i = 0; i < count; ++i) {
    MIDIINCAPSW caps{};
    if (midiInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
      names.push_back(toUtf8(caps.szPname));
    } else {
      names.push_back("MIDI Input " + std::to_string(i + 1));
    }
  }
  return names;
}

bool MidiInput::open(int deviceIndex) {
  close();
  error_.clear();
  index_ = deviceIndex;

  const UINT deviceId = deviceIndex < 0 ? (UINT)MIDI_MAPPER : (UINT)deviceIndex;
  const MMRESULT result = midiInOpen(&handle_, deviceId, (DWORD_PTR)&MidiInput::midiCallback,
                                     (DWORD_PTR)this,
                                     CALLBACK_FUNCTION | MIDI_IO_STATUS);
  if (result != MMSYSERR_NOERROR) {
    handle_ = nullptr;
    error_ = "cannot open MIDI input";
    return false;
  }
  midiInStart(handle_);
  return true;
}

void MidiInput::close() {
  if (!handle_) return;
  midiInStop(handle_);
  midiInReset(handle_);
  midiInClose(handle_);
  handle_ = nullptr;
}

std::string MidiInput::deviceName() const {
  const auto names = deviceNames();
  if (index_ >= 0 && index_ < (int)names.size()) return names[(size_t)index_];
  return {};
}

bool MidiInput::poll(MidiMessage& out) { return queue_.pop(out); }

void CALLBACK MidiInput::midiCallback(HMIDIIN handle, UINT msg, DWORD_PTR user, DWORD_PTR param1,
                                      DWORD_PTR param2) {
  (void)handle;
  (void)param2;
  if (msg != MIM_DATA) return;
  auto* self = reinterpret_cast<MidiInput*>(user);
  if (!self) return;
  self->onShortMessage((DWORD)param1);
}

void MidiInput::onShortMessage(DWORD packed) {
  const int status = (int)(packed & 0xFF);
  const int data1 = (int)((packed >> 8) & 0xFF);
  const int data2 = (int)((packed >> 16) & 0xFF);
  const int kind = status & 0xF0;
  // Keep only channel voice messages we care about.
  if (kind == 0x90 || kind == 0x80 || kind == 0xB0 || kind == 0xE0 || kind == 0xC0 ||
      kind == 0xA0) {
    MidiMessage message{status, data1, data2};
    queue_.push(message);
  }
}

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------

MidiOutput::~MidiOutput() { close(); }

std::vector<std::string> MidiOutput::deviceNames() {
  std::vector<std::string> names;
  const UINT count = midiOutGetNumDevs();
  for (UINT i = 0; i < count; ++i) {
    MIDIOUTCAPSW caps{};
    if (midiOutGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
      names.push_back(toUtf8(caps.szPname));
    } else {
      names.push_back("MIDI Output " + std::to_string(i + 1));
    }
  }
  return names;
}

bool MidiOutput::open(int deviceIndex) {
  close();
  index_ = deviceIndex;
  const UINT deviceId = deviceIndex < 0 ? (UINT)MIDI_MAPPER : (UINT)deviceIndex;
  if (midiOutOpen(&handle_, deviceId, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
    handle_ = nullptr;
    return false;
  }
  return true;
}

void MidiOutput::close() {
  if (!handle_) return;
  allNotesOff();
  midiOutReset(handle_);
  midiOutClose(handle_);
  handle_ = nullptr;
}

void MidiOutput::send(int status, int data1, int data2) {
  if (!handle_) return;
  const DWORD packed = (DWORD)(status & 0xFF) | ((DWORD)(data1 & 0x7F) << 8) |
                       ((DWORD)(data2 & 0x7F) << 16);
  midiOutShortMsg(handle_, packed);
}

void MidiOutput::allNotesOff() {
  if (!handle_) return;
  for (int channel = 0; channel < 16; ++channel) {
    const DWORD packed = (DWORD)(0xB0 | channel) | (123u << 8) | (0u << 16);
    midiOutShortMsg(handle_, packed);
  }
}

} // namespace nr
