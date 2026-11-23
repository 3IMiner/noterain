// NoteRain Native — application state, transport, practice engine
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <windows.h>

#include "audio/engine.h"
#include "audio/synth.h"
#include "core/song.h"
#include "i18n.h"
#include "midi/midiio.h"
#include "ui/canvas.h"
#include "ui/theme.h"

namespace nr {

struct Settings {
  int midiInput = 0;     // 0 = none, otherwise device index + 1
  int midiOutput = 0;    // 0 = off
  int midiChannel = 0;   // 0 = all, 1..16
  int velocityCurve = 0; // 0 linear, 1 soft, 2 hard, 3 fixed
  int transpose = 0;
  int latencyMs = 0;
  float volume = 0.80f;
  float metronomeVolume = 0.45f;
  bool metronome = false;
  int lookahead = 3;
  bool noteNames = true;
  int theme = 0;
  int language = 0; // 0 zh, 1 en
  bool practiceMode = true;
  int practiceHand = 0; // 0 both, 1 left, 2 right
  bool waitMode = true;
  int assist = 1; // 0 never, 1 after 8s, 2 after 4s
  float speed = 0.75f;
  bool autoSpeed = false;
  bool loop = false;
  int loopStart = 0;
  int loopEnd = 3;

  // --- keyboard window (for 25/32/37/49-key controllers) ---
  int keyCount = 88;      // visible keys: 25/32/37/49/61/88
  int keyLow = 21;        // user's preferred lowest visible key
  bool autoFollow = true; // shift the window so the next notes stay reachable
  bool alignHardware = false; // map hardware key 0 onto the window's lowest key
  int hardwareLow = 48;       // MIDI note produced by the lowest physical key
  bool outOfRangeAccomp = true; // notes outside the window play themselves

  std::string lastFile;
};

class App {
public:
  bool init(HWND hwnd);
  void shutdown();

  void frame();
  void resize(int pixelWidth, int pixelHeight, float dpiScale);

  void onMouseMove(int x, int y);
  void onMouseDown();
  void onMouseUp();
  void onMouseWheel(int delta);
  void onKeyDown(int vk, bool ctrlDown);
  void onDropFile(const std::wstring& path);

  bool exitRequested() const { return wantExit_; }

private:
  // --- setup / persistence
  bool loadSettings();
  void saveSettings();
  void applySettings();
  void refreshDevices();
  void openInputDevice();
  void openOutputDevice();
  void applyWindowTheme();
  void openFileDialog();

  // --- song handling
  bool loadSongFile(const std::filesystem::path& path);
  void loadDemoSong();
  void onSongLoaded();
  void rebuildTargets();
  void resetCursorToTime();
  void seekTo(double seconds);

  // --- transport
  void togglePlay();
  void stopPlayback();
  void updateTransport(double dt);
  void scheduleAuto(double fromTime, double toTime);
  void flushPendingOffs();
  void updateMetronome();
  bool loopActive() const;
  double loopStartTime() const;
  double loopEndTime() const;

  // --- input
  void handleMidiMessage(const MidiMessage& message);
  void noteOnFromUser(int key, int velocity);
  void noteOffFromUser(int key);
  int applyVelocityCurve(int velocity) const;

  // --- keyboard window
  int windowHigh() const;
  int windowKeyCount() const;
  void resetKeyboardWindow();
  void ensureKeyVisible(int key);
  void shiftWindow(int octaves);
  int mapHardwareKey(int key) const;

  // --- practice
  bool practiceActive() const;
  bool matchesHandAndTrack(const Note* note) const;
  bool isTargetNote(const Note* note) const;
  void updatePractice();
  void startGroup(size_t index);
  void completeGroup();
  void skipGroup();
  void trySatisfy(int key);
  void resetStats(bool keepCursor);
  double accuracy() const;

  // --- drawing
  const Theme& theme() const { return themeAt(settings_.theme); }
  void markSettingsDirty() { settingsDirty_ = true; }
  void computeLayout();
  void draw();
  void drawTopBar();
  void drawSongArea();
  void drawKeyboard();
  /** Miniature A0-C8 keyboard: current window + hardware coverage + note dots. */
  void drawRangeOverview(Rect area, bool interactive, bool big);
  void drawPanel();
  void drawEmptyState();
  void drawSettings();
  void drawProgressBar(Rect area);

  HWND hwnd_ = nullptr;
  Canvas canvas_;
  Settings settings_;
  Lang lang_ = Lang::Zh;
  const Strings* str_ = nullptr;

  Song song_;
  PianoSynth synth_;
  AudioEngine audio_;
  MidiInput midiIn_;
  MidiOutput midiOut_;

  std::vector<std::string> inputDevices_;
  std::vector<std::string> outputDevices_;
  std::string audioStatus_;
  std::string errorBanner_;
  bool audioOk_ = false;

  // Transport
  bool playing_ = false;
  double time_ = 0.0;
  double wallClock_ = 0.0;
  double lastFrameWall_ = 0.0;
  int autoCursor_ = 0;
  struct PendingOff {
    double when = 0.0;
    int key = 0;
  };
  std::vector<PendingOff> pendingOffs_;
  int lastBeat_ = -1;
  int loopPass_ = 0;

  // View
  enum class View { Player, Settings };
  View view_ = View::Player;

  // Practice
  std::vector<const Note*> targets_;
  size_t cursor_ = 0;
  size_t groupEnd_ = 0;
  std::vector<int> pendingKeys_;
  double groupStart_ = 0.0;
  bool waiting_ = false;
  double waitStartWall_ = 0.0;
  int hits_ = 0;
  int wrong_ = 0;
  int skipped_ = 0;
  int combo_ = 0;
  int bestCombo_ = 0;
  double lastDeltaMs_ = 0.0;
  bool sectionDone_ = false;

  // Keyboard visuals
  bool pressedKeys_[128] = {};
  bool autoKeys_[128] = {};
  double flash_[128] = {};
  int mouseKey_ = -1;
  int hwPressedNote_[128] = {}; // hardware key -> mapped note (or -1)

  // Keyboard window (runtime; follows the music when auto-follow is on)
  int windowLow_ = 21;
  bool scrubbingRange_ = false;
  float settingsScroll_ = 0.0f;

  // Layout
  Rect topBarArea_;
  Rect notesArea_;
  Rect keyboardArea_;
  Rect panelArea_;

  bool wantExit_ = false;
  bool settingsDirty_ = false;
};

} // namespace nr
