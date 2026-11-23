#include "app/app.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>

#include <commdlg.h>
#include <dwmapi.h>

#include "core/smf.h"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace nr {
namespace {

double wallSeconds() {
  static const auto start = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

std::string toUtf8(const std::wstring& wide) {
  if (wide.empty()) return {};
  const int size =
      WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
  if (size <= 0) return {};
  std::string out((size_t)size, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), out.data(), size, nullptr,
                      nullptr);
  return out;
}

std::wstring toWide(const std::string& utf8) {
  if (utf8.empty()) return {};
  const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
  if (size <= 0) return {};
  std::wstring out((size_t)size, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), out.data(), size);
  return out;
}

std::filesystem::path configPath() {
  wchar_t buffer[MAX_PATH * 2]{};
  const DWORD length = GetEnvironmentVariableW(L"APPDATA", buffer, (DWORD)std::size(buffer));
  std::filesystem::path dir;
  if (length > 0 && length < std::size(buffer)) dir = std::filesystem::path(buffer);
  else dir = std::filesystem::temp_directory_path();
  dir /= L"NoteRainNative";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  return dir / L"settings.ini";
}

int toInt(const std::string& text, int fallback) {
  try {
    return std::stoi(text);
  } catch (...) {
    return fallback;
  }
}

float toFloat(const std::string& text, float fallback) {
  try {
    return std::stof(text);
  } catch (...) {
    return fallback;
  }
}

} // namespace

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

bool App::init(HWND hwnd) {
  hwnd_ = hwnd;
  if (!canvas_.init(hwnd)) return false;

  for (int i = 0; i < 128; ++i) hwPressedNote_[i] = -1;

  loadSettings();
  lang_ = settings_.language == 1 ? Lang::En : Lang::Zh;
  str_ = &strings(lang_);

  refreshDevices();
  openInputDevice();
  openOutputDevice();

  audio_.start(&synth_);
  audioOk_ = audio_.running();
  audioStatus_ = audioOk_ ? str_->audioRunning
                          : (audio_.error().empty() ? str_->audioStopped : audio_.error());

  applySettings();
  applyWindowTheme();
  resetKeyboardWindow();

  if (!settings_.lastFile.empty()) {
    std::error_code ec;
    const std::filesystem::path path(toWide(settings_.lastFile));
    if (std::filesystem::exists(path, ec)) loadSongFile(path);
  }

  lastFrameWall_ = wallSeconds();
  return true;
}

void App::shutdown() {
  saveSettings();
  synth_.queueAllOff();
  audio_.stop();
  midiIn_.close();
  midiOut_.close();
  canvas_.shutdown();
}

void App::resize(int pixelWidth, int pixelHeight, float dpiScale) {
  canvas_.resize((UINT)pixelWidth, (UINT)pixelHeight, dpiScale);
}

bool App::loadSettings() {
  std::ifstream in(configPath());
  if (!in) return false;
  std::string line;
  while (std::getline(in, line)) {
    const size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = line.substr(0, eq);
    const std::string value = line.substr(eq + 1);
    if (key == "midiInput") settings_.midiInput = toInt(value, 0);
    else if (key == "midiOutput") settings_.midiOutput = toInt(value, 0);
    else if (key == "midiChannel") settings_.midiChannel = toInt(value, 0);
    else if (key == "velocityCurve") settings_.velocityCurve = toInt(value, 0);
    else if (key == "transpose") settings_.transpose = toInt(value, 0);
    else if (key == "latencyMs") settings_.latencyMs = toInt(value, 0);
    else if (key == "volume") settings_.volume = toFloat(value, 0.8f);
    else if (key == "metronomeVolume") settings_.metronomeVolume = toFloat(value, 0.45f);
    else if (key == "metronome") settings_.metronome = toInt(value, 0) != 0;
    else if (key == "lookahead") settings_.lookahead = toInt(value, 3);
    else if (key == "noteNames") settings_.noteNames = toInt(value, 1) != 0;
    else if (key == "theme") settings_.theme = toInt(value, 0);
    else if (key == "language") settings_.language = toInt(value, 0);
    else if (key == "practiceMode") settings_.practiceMode = toInt(value, 1) != 0;
    else if (key == "practiceHand") settings_.practiceHand = toInt(value, 0);
    else if (key == "waitMode") settings_.waitMode = toInt(value, 1) != 0;
    else if (key == "assist") settings_.assist = toInt(value, 1);
    else if (key == "speed") settings_.speed = toFloat(value, 0.75f);
    else if (key == "autoSpeed") settings_.autoSpeed = toInt(value, 0) != 0;
    else if (key == "loop") settings_.loop = toInt(value, 0) != 0;
    else if (key == "loopStart") settings_.loopStart = toInt(value, 0);
    else if (key == "loopEnd") settings_.loopEnd = toInt(value, 3);
    else if (key == "keyCount") settings_.keyCount = toInt(value, 88);
    else if (key == "keyLow") settings_.keyLow = toInt(value, 21);
    else if (key == "autoFollow") settings_.autoFollow = toInt(value, 1) != 0;
    else if (key == "alignHardware") settings_.alignHardware = toInt(value, 0) != 0;
    else if (key == "hardwareLow") settings_.hardwareLow = toInt(value, 48);
    else if (key == "outOfRangeAccomp") settings_.outOfRangeAccomp = toInt(value, 1) != 0;
    else if (key == "lastFile") settings_.lastFile = value;
  }
  settings_.volume = std::clamp(settings_.volume, 0.0f, 1.0f);
  settings_.metronomeVolume = std::clamp(settings_.metronomeVolume, 0.0f, 1.0f);
  settings_.speed = std::clamp(settings_.speed, 0.25f, 1.5f);
  settings_.lookahead = std::clamp(settings_.lookahead, 1, 6);
  settings_.transpose = std::clamp(settings_.transpose, -24, 24);
  settings_.latencyMs = std::clamp(settings_.latencyMs, -200, 200);
  settings_.midiChannel = std::clamp(settings_.midiChannel, 0, 16);
  settings_.practiceHand = std::clamp(settings_.practiceHand, 0, 2);
  settings_.assist = std::clamp(settings_.assist, 0, 2);
  settings_.theme = std::clamp(settings_.theme, 0, 1);
  settings_.language = std::clamp(settings_.language, 0, 1);
  settings_.keyCount = std::clamp(settings_.keyCount, 25, 88);
  settings_.keyLow = std::clamp(settings_.keyLow, 21, 108);
  settings_.hardwareLow = std::clamp(settings_.hardwareLow, 0, 108);
  return true;
}

void App::saveSettings() {
  std::ofstream out(configPath(), std::ios::trunc);
  if (!out) return;
  out << "midiInput=" << settings_.midiInput << "\n";
  out << "midiOutput=" << settings_.midiOutput << "\n";
  out << "midiChannel=" << settings_.midiChannel << "\n";
  out << "velocityCurve=" << settings_.velocityCurve << "\n";
  out << "transpose=" << settings_.transpose << "\n";
  out << "latencyMs=" << settings_.latencyMs << "\n";
  out << "volume=" << settings_.volume << "\n";
  out << "metronomeVolume=" << settings_.metronomeVolume << "\n";
  out << "metronome=" << (settings_.metronome ? 1 : 0) << "\n";
  out << "lookahead=" << settings_.lookahead << "\n";
  out << "noteNames=" << (settings_.noteNames ? 1 : 0) << "\n";
  out << "theme=" << settings_.theme << "\n";
  out << "language=" << settings_.language << "\n";
  out << "practiceMode=" << (settings_.practiceMode ? 1 : 0) << "\n";
  out << "practiceHand=" << settings_.practiceHand << "\n";
  out << "waitMode=" << (settings_.waitMode ? 1 : 0) << "\n";
  out << "assist=" << settings_.assist << "\n";
  out << "speed=" << settings_.speed << "\n";
  out << "autoSpeed=" << (settings_.autoSpeed ? 1 : 0) << "\n";
  out << "loop=" << (settings_.loop ? 1 : 0) << "\n";
  out << "loopStart=" << settings_.loopStart << "\n";
  out << "loopEnd=" << settings_.loopEnd << "\n";
  out << "keyCount=" << settings_.keyCount << "\n";
  out << "keyLow=" << settings_.keyLow << "\n";
  out << "autoFollow=" << (settings_.autoFollow ? 1 : 0) << "\n";
  out << "alignHardware=" << (settings_.alignHardware ? 1 : 0) << "\n";
  out << "hardwareLow=" << settings_.hardwareLow << "\n";
  out << "outOfRangeAccomp=" << (settings_.outOfRangeAccomp ? 1 : 0) << "\n";
  out << "lastFile=" << settings_.lastFile << "\n";
}

void App::applySettings() {
  lang_ = settings_.language == 1 ? Lang::En : Lang::Zh;
  str_ = &strings(lang_);
  synth_.setMasterGain(settings_.volume);
  synth_.setMetronomeGain(settings_.metronomeVolume);
}

void App::refreshDevices() {
  inputDevices_ = MidiInput::deviceNames();
  outputDevices_ = MidiOutput::deviceNames();
  if (settings_.midiInput > (int)inputDevices_.size()) settings_.midiInput = 0;
  if (settings_.midiOutput > (int)outputDevices_.size()) settings_.midiOutput = 0;
}

void App::openInputDevice() {
  midiIn_.close();
  if (settings_.midiInput <= 0 || settings_.midiInput > (int)inputDevices_.size()) return;
  midiIn_.open(settings_.midiInput - 1);
}

void App::openOutputDevice() {
  midiOut_.close();
  if (settings_.midiOutput <= 0 || settings_.midiOutput > (int)outputDevices_.size()) return;
  midiOut_.open(settings_.midiOutput - 1);
}

void App::applyWindowTheme() {
  const BOOL dark = settings_.theme == 0 ? TRUE : FALSE;
  DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  BOOL dark2 = dark;
  DwmSetWindowAttribute(hwnd_, 19 /* DWMWA_USE_IMMERSIVE_DARK_MODE (pre-20H1) */, &dark2,
                        sizeof(dark2));
}

void App::openFileDialog() {
  wchar_t file[MAX_PATH * 2]{};
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd_;
  ofn.lpstrFilter = lang_ == Lang::En
                        ? L"MIDI files\0*.mid;*.midi\0All files\0*.*\0\0"
                        : L"MIDI 文件\0*.mid;*.midi\0所有文件\0*.*\0\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = (DWORD)std::size(file);
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
  if (GetOpenFileNameW(&ofn)) loadSongFile(std::filesystem::path(file));
}

// ---------------------------------------------------------------------------
// Song
// ---------------------------------------------------------------------------

bool App::loadSongFile(const std::filesystem::path& path) {
  Song loaded;
  std::string error;
  if (!loadMidiFile(path, loaded, error)) {
    errorBanner_ = error;
    return false;
  }
  song_ = std::move(loaded);
  settings_.lastFile = toUtf8(path.wstring());
  saveSettings();
  onSongLoaded();
  errorBanner_.clear();
  return true;
}

void App::loadDemoSong() {
  song_ = makeDemoSong();
  settings_.lastFile.clear();
  onSongLoaded();
  errorBanner_.clear();
}

void App::onSongLoaded() {
  playing_ = false;
  time_ = 0.0;
  resetKeyboardWindow();
  settings_.loopStart = 0;
  settings_.loopEnd = std::clamp(song_.measureCount() - 1, 0, 3);
  settings_.loop = false;
  view_ = View::Player;
  rebuildTargets();
  resetStats(false);
  resetCursorToTime();
  synth_.queueAllOff();
  midiOut_.allNotesOff();
  SetWindowTextW(hwnd_, (std::wstring(L"NoteRain — ") + toWide(song_.title)).c_str());
}

void App::rebuildTargets() {
  targets_.clear();
  for (const Note* note : song_.ordered) {
    if (matchesHandAndTrack(note)) targets_.push_back(note);
  }
  waiting_ = false;
  pendingKeys_.clear();
  cursor_ = 0;
}

void App::resetCursorToTime() {
  waiting_ = false;
  pendingKeys_.clear();
  sectionDone_ = false;
  lastBeat_ = -1;

  cursor_ = 0;
  while (cursor_ < targets_.size() && targets_[cursor_]->start < time_ - 0.02) ++cursor_;

  autoCursor_ = 0;
  while (autoCursor_ < (int)song_.ordered.size() &&
         song_.ordered[(size_t)autoCursor_]->start < time_ - 0.02)
    ++autoCursor_;

  pendingOffs_.clear();
  for (int i = 0; i < 128; ++i) {
    pressedKeys_[i] = false;
    autoKeys_[i] = false;
    flash_[i] = 0.0;
  }
}

void App::seekTo(double seconds) {
  const double maxTime = song_.empty() ? 0.0 : song_.duration;
  time_ = std::clamp(seconds, 0.0, maxTime);
  resetCursorToTime();
}

// ---------------------------------------------------------------------------
// Keyboard window (25/32/37-key controllers)
// ---------------------------------------------------------------------------

int App::windowKeyCount() const { return std::clamp(settings_.keyCount, 25, 88); }

int App::windowHigh() const { return std::min(108, windowLow_ + windowKeyCount() - 1); }

void App::resetKeyboardWindow() {
  const int count = windowKeyCount();
  const int maxLow = std::max(21, 108 - count + 1);
  int low = settings_.keyLow;

  if (!song_.empty() && song_.maxKey >= song_.minKey) {
    const int span = song_.maxKey - song_.minKey + 1;
    // Fits entirely? Centre it a little below the lowest note; otherwise start
    // at the song's lowest note and let auto-follow walk upwards.
    low = span < count ? song_.minKey - 1 : song_.minKey;
  }
  windowLow_ = std::clamp(low, 21, maxLow);
}

void App::ensureKeyVisible(int key) {
  if (!settings_.autoFollow) return;
  const int count = windowKeyCount();
  const int low = windowLow_;
  const int high = windowHigh();
  if (key >= low && key <= high) return;

  int delta = 0;
  if (key < low) {
    delta = -12 * ((low - key + 11) / 12);
  } else {
    delta = 12 * ((key - high + 11) / 12);
  }
  const int maxLow = std::max(21, 108 - count + 1);
  const int next = std::clamp(low + delta, 21, maxLow);
  if (next == windowLow_) return;

  windowLow_ = next;
  // With hardware alignment the physical keys point somewhere else now, so
  // release anything still held to avoid stuck notes.
  for (int i = 0; i < 128; ++i) {
    if (hwPressedNote_[i] >= 0) {
      noteOffFromUser(hwPressedNote_[i]);
      hwPressedNote_[i] = -1;
    }
  }
}

void App::shiftWindow(int octaves) {
  const int count = windowKeyCount();
  const int maxLow = std::max(21, 108 - count + 1);
  windowLow_ = std::clamp(windowLow_ + octaves * 12, 21, maxLow);
  settings_.keyLow = windowLow_;
  markSettingsDirty();
}

int App::mapHardwareKey(int key) const {
  int offset = 0;
  if (settings_.alignHardware) offset += windowLow_ - settings_.hardwareLow;
  offset += settings_.transpose;
  const int mapped = key + offset;
  if (mapped < 0 || mapped > 127) return -1;
  return mapped;
}

// ---------------------------------------------------------------------------
// Transport
// ---------------------------------------------------------------------------

void App::togglePlay() {
  if (song_.empty()) return;
  if (playing_) {
    playing_ = false;
    flushPendingOffs();
    synth_.queueAllOff();
    midiOut_.allNotesOff();
    return;
  }
  if (time_ >= song_.duration - 0.05) seekTo(loopActive() ? loopStartTime() : 0.0);
  playing_ = true;
  lastFrameWall_ = wallSeconds();
  resetCursorToTime();
  if (time_ <= 0.01) resetStats(false);
}

void App::stopPlayback() {
  playing_ = false;
  synth_.queueAllOff();
  midiOut_.allNotesOff();
  seekTo(loopActive() ? loopStartTime() : 0.0);
  resetStats(false);
}

bool App::loopActive() const { return settings_.loop && song_.measureCount() > 1; }

double App::loopStartTime() const { return song_.measureStart(settings_.loopStart); }

double App::loopEndTime() const {
  return song_.measureStart(std::min(settings_.loopEnd + 1, song_.measureCount()));
}

void App::updateTransport(double dt) {
  if (!playing_) return;
  if (practiceActive() && waiting_) return; // practice holds the clock

  const double previous = time_;
  const double next = time_ + dt * settings_.speed;

  if (loopActive()) {
    const double endTime = loopEndTime();
    if (next >= endTime) {
      time_ = loopStartTime();
      ++loopPass_;
      if (settings_.autoSpeed) settings_.speed = std::min(1.5f, settings_.speed + 0.05f);
      resetCursorToTime();
      return;
    }
    time_ = next;
    scheduleAuto(previous, time_);
    return;
  }

  time_ = std::min(next, song_.duration);
  scheduleAuto(previous, time_);
  if (time_ >= song_.duration) {
    playing_ = false;
    synth_.queueAllOff();
    midiOut_.allNotesOff();
  }
}

void App::scheduleAuto(double fromTime, double toTime) {
  if (song_.empty()) return;
  const int channel = settings_.midiChannel > 0 ? settings_.midiChannel - 1 : 0;

  while (autoCursor_ < (int)song_.ordered.size()) {
    const Note* note = song_.ordered[(size_t)autoCursor_];
    if (note->start > toTime) break;
    ++autoCursor_;
    if (note->start < fromTime - 0.001) continue;
    if (!song_.tracks[note->track].enabled) continue;
    if (practiceActive() && isTargetNote(note)) continue; // the player performs these

    const int key = std::clamp(note->key, 0, 127);
    // Listening mode: let the window follow the music like a page turner.
    if (!practiceActive()) ensureKeyVisible(key);
    synth_.queueNoteOn(key, note->velocity);
    autoKeys_[key] = true;
    pendingOffs_.push_back({note->end(), key});
    if (midiOut_.isOpen()) midiOut_.send(0x90 | channel, key, note->velocity);
  }
}

void App::flushPendingOffs() {
  if (pendingOffs_.empty()) return;
  const int channel = settings_.midiChannel > 0 ? settings_.midiChannel - 1 : 0;
  std::vector<PendingOff> keep;
  keep.reserve(pendingOffs_.size());
  for (const auto& off : pendingOffs_) {
    if (!playing_ || off.when <= time_ + 0.0005) {
      synth_.queueNoteOff(off.key);
      autoKeys_[off.key] = false;
      if (midiOut_.isOpen()) midiOut_.send(0x80 | channel, off.key, 0);
    } else {
      keep.push_back(off);
    }
  }
  pendingOffs_.swap(keep);
}

void App::updateMetronome() {
  if (!playing_ || !settings_.metronome || song_.empty()) return;
  const double beat = song_.beatAt(time_);
  const int beatIndex = (int)std::floor(beat + 1e-6);
  if (beatIndex == lastBeat_) return;
  lastBeat_ = beatIndex;
  const int beatsPerBar = std::max(1, song_.timeSigAt(time_).num);
  synth_.queueClick(beatIndex % beatsPerBar == 0);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

int App::applyVelocityCurve(int velocity) const {
  const double v = std::clamp(velocity / 127.0, 0.01, 1.0);
  switch (settings_.velocityCurve) {
    case 1: return (int)std::lround(std::pow(v, 0.68) * 127.0);
    case 2: return (int)std::lround(std::pow(v, 1.55) * 127.0);
    case 3: return 100;
    default: return (int)std::lround(v * 127.0);
  }
}

void App::handleMidiMessage(const MidiMessage& message) {
  const int kind = message.status & 0xF0;
  const int channel = message.status & 0x0F;

  if (settings_.midiChannel > 0) {
    if (channel != settings_.midiChannel - 1) {
      if (kind != 0xB0) return;
      if (message.data1 != 123 && message.data1 != 64) return;
    }
  }

  const int hardwareKey = message.data1 & 0x7F;

  if (kind == 0x90 && message.data2 > 0) {
    const int mapped = mapHardwareKey(hardwareKey);
    if (mapped < 0) return;
    hwPressedNote_[hardwareKey] = mapped;
    noteOnFromUser(mapped, message.data2);
  } else if (kind == 0x80 || (kind == 0x90 && message.data2 == 0)) {
    const int mapped = hwPressedNote_[hardwareKey];
    hwPressedNote_[hardwareKey] = -1;
    if (mapped >= 0) noteOffFromUser(mapped);
  } else if (kind == 0xB0) {
    if (message.data1 == 64) {
      synth_.queuePedal(message.data2 >= 64);
    } else if (message.data1 == 123 || message.data1 == 120) {
      for (int i = 0; i < 128; ++i) {
        if (hwPressedNote_[i] >= 0) {
          noteOffFromUser(hwPressedNote_[i]);
          hwPressedNote_[i] = -1;
        }
      }
      synth_.queueAllOff();
    }
  }
}

void App::noteOnFromUser(int key, int velocity) {
  if (key < 0 || key > 127) return;
  const int shaped = applyVelocityCurve(velocity);

  synth_.queueNoteOn(key, shaped);
  pressedKeys_[key] = true;
  if (midiOut_.isOpen()) {
    const int channel = settings_.midiChannel > 0 ? settings_.midiChannel - 1 : 0;
    midiOut_.send(0x90 | channel, key, shaped);
  }
  if (practiceActive()) trySatisfy(key);
}

void App::noteOffFromUser(int key) {
  if (key < 0 || key > 127) return;
  synth_.queueNoteOff(key);
  pressedKeys_[key] = false;
  if (midiOut_.isOpen()) {
    const int channel = settings_.midiChannel > 0 ? settings_.midiChannel - 1 : 0;
    midiOut_.send(0x80 | channel, key, 0);
  }
}

void App::onMouseMove(int x, int y) { canvas_.onMouseMove((float)x, (float)y); }
void App::onMouseDown() { canvas_.onMouseDown(); }
void App::onMouseUp() {
  canvas_.onMouseUp();
  if (mouseKey_ >= 0) {
    noteOffFromUser(mouseKey_);
    mouseKey_ = -1;
  }
}
void App::onMouseWheel(int delta) { canvas_.onWheel(delta); }

void App::onKeyDown(int vk, bool ctrlDown) {
  switch (vk) {
    case VK_SPACE:
      togglePlay();
      break;
    case VK_LEFT:
      seekTo(time_ - 5.0);
      break;
    case VK_RIGHT:
      seekTo(time_ + 5.0);
      break;
    case VK_UP:
      settings_.speed = std::min(1.5f, settings_.speed + 0.05f);
      markSettingsDirty();
      break;
    case VK_DOWN:
      settings_.speed = std::max(0.25f, settings_.speed - 0.05f);
      markSettingsDirty();
      break;
    case 'L':
      settings_.loop = !settings_.loop;
      markSettingsDirty();
      break;
    case 'M':
      settings_.metronome = !settings_.metronome;
      markSettingsDirty();
      break;
    case 'W':
      settings_.practiceMode = !settings_.practiceMode;
      resetStats(false);
      resetCursorToTime();
      markSettingsDirty();
      break;
    case VK_OEM_4: // [
      shiftWindow(-1);
      break;
    case VK_OEM_6: // ]
      shiftWindow(1);
      break;
    case 'S':
      if (!ctrlDown) view_ = view_ == View::Settings ? View::Player : View::Settings;
      break;
    case VK_ESCAPE:
      view_ = View::Player;
      break;
    case 'O':
      if (ctrlDown) openFileDialog();
      break;
    default:
      break;
  }
}

void App::onDropFile(const std::wstring& path) { loadSongFile(std::filesystem::path(path)); }

// ---------------------------------------------------------------------------
// Practice
// ---------------------------------------------------------------------------

bool App::practiceActive() const {
  return !song_.empty() && view_ != View::Settings && settings_.practiceMode;
}

bool App::matchesHandAndTrack(const Note* note) const {
  if (!note) return false;
  if (note->track < 0 || note->track >= (int)song_.tracks.size()) return false;
  if (!song_.tracks[(size_t)note->track].enabled) return false;
  switch (settings_.practiceHand) {
    case 1: return note->leftHand();
    case 2: return !note->leftHand();
    default: return true;
  }
}

bool App::isTargetNote(const Note* note) const {
  if (!matchesHandAndTrack(note)) return false;
  if (settings_.outOfRangeAccomp) {
    if (note->key < windowLow_ || note->key > windowHigh()) return false;
  }
  return true;
}

void App::resetStats(bool keepCursor) {
  hits_ = 0;
  wrong_ = 0;
  skipped_ = 0;
  combo_ = 0;
  bestCombo_ = 0;
  lastDeltaMs_ = 0.0;
  sectionDone_ = false;
  if (!keepCursor) {
    cursor_ = 0;
    waiting_ = false;
    pendingKeys_.clear();
  }
}

double App::accuracy() const {
  const int total = hits_ + wrong_ + skipped_;
  if (total == 0) return 100.0;
  return 100.0 * (double)hits_ / (double)total;
}

void App::startGroup(size_t index) {
  if (index >= targets_.size()) return;
  groupStart_ = targets_[index]->start;
  time_ = groupStart_;
  pendingKeys_.clear();
  size_t i = index;
  while (i < targets_.size() && targets_[i]->start <= groupStart_ + 0.06) {
    pendingKeys_.push_back(targets_[i]->key);
    ++i;
  }
  groupEnd_ = i;
  cursor_ = index;
  waiting_ = true;
  waitStartWall_ = wallClock_;
}

void App::completeGroup() {
  waiting_ = false;
  pendingKeys_.clear();
  cursor_ = groupEnd_;
  time_ = groupStart_ + 0.002;
  if (cursor_ >= targets_.size()) sectionDone_ = true;
}

void App::skipGroup() {
  skipped_ += (int)pendingKeys_.size();
  combo_ = 0;
  waiting_ = false;
  pendingKeys_.clear();
  cursor_ = groupEnd_;
  time_ = groupStart_ + 0.002;
  if (cursor_ >= targets_.size()) sectionDone_ = true;
}

void App::trySatisfy(int key) {
  if (!waiting_) return;
  auto it = std::find(pendingKeys_.begin(), pendingKeys_.end(), key);
  if (it == pendingKeys_.end()) {
    ++wrong_;
    combo_ = 0;
    flash_[key] = -1.0; // negative = wrong (red)
    return;
  }
  pendingKeys_.erase(it);
  ++hits_;
  ++combo_;
  bestCombo_ = std::max(bestCombo_, combo_);
  lastDeltaMs_ = (wallClock_ - waitStartWall_) * 1000.0 - (double)settings_.latencyMs;
  flash_[key] = 1.0;
  if (pendingKeys_.empty()) completeGroup();
}

void App::updatePractice() {
  if (!practiceActive() || !playing_) return;

  // Keep the upcoming notes inside the visible window (small keyboards).
  if (!waiting_) {
    for (size_t i = cursor_; i < targets_.size() && i < cursor_ + 4; ++i) {
      ensureKeyVisible(targets_[i]->key);
    }
  }

  if (waiting_) {
    time_ = groupStart_;
    const double waited = wallClock_ - waitStartWall_;
    if (settings_.assist == 2 && waited > 4.0) skipGroup();
    else if (settings_.assist == 1 && waited > 8.0) skipGroup();
    return;
  }

  // Notes that are unreachable on this keyboard window are performed for the
  // player (accompaniment) instead of being demanded.
  while (cursor_ < targets_.size() && !isTargetNote(targets_[cursor_])) ++cursor_;

  while (cursor_ < targets_.size() && targets_[cursor_]->start < time_ - 0.6) ++cursor_;
  if (cursor_ >= targets_.size()) {
    if (!targets_.empty()) sectionDone_ = true;
    return;
  }
  if (targets_[cursor_]->start <= time_ + 0.004) startGroup(cursor_);
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void App::frame() {
  wallClock_ = wallSeconds();
  double dt = wallClock_ - lastFrameWall_;
  lastFrameWall_ = wallClock_;
  if (dt < 0.0) dt = 0.0;
  if (dt > 0.1) dt = 0.1;

  MidiMessage message;
  while (midiIn_.poll(message)) handleMidiMessage(message);

  updateTransport(dt);
  updatePractice();
  flushPendingOffs();
  updateMetronome();

  if (settingsDirty_) {
    settingsDirty_ = false;
    applySettings();
    saveSettings();
    applyWindowTheme();
  }

  computeLayout();
  canvas_.beginFrame();
  draw();
  canvas_.endFrame();
}

void App::computeLayout() {
  const float width = canvas_.width();
  const float height = canvas_.height();
  const float topBarHeight = 64.0f;
  const float keyboardHeight = 148.0f;
  const float panelWidth = (!song_.empty() && view_ == View::Player) ? 304.0f : 0.0f;

  topBarArea_ = Rect(0.0f, 0.0f, width, topBarHeight);
  keyboardArea_ = Rect(0.0f, height - keyboardHeight, width - panelWidth, keyboardHeight);
  notesArea_ = Rect(0.0f, topBarHeight, width - panelWidth, height - topBarHeight - keyboardHeight);
  panelArea_ = Rect(width - panelWidth, topBarHeight, panelWidth, height - topBarHeight);
}

} // namespace nr
