// NoteRain Native — UI strings (Simplified Chinese / English)
#pragma once

namespace nr {

enum class Lang { Zh = 0, En = 1 };

struct Strings {
  // Shell
  const char* appName;
  const char* openMidi;
  const char* openDialogTitle;
  const char* demoTitle;
  const char* noFileTitle;
  const char* noFileBody;
  const char* formatsHint;
  const char* shortcutHint;
  const char* settings;
  const char* back;
  const char* close;
  const char* dropHint;

  // Transport
  const char* play;
  const char* pause;
  const char* stop;
  const char* speed;
  const char* metronome;
  const char* listen;
  const char* practice;
  const char* bar;

  // Hands
  const char* bothHands;
  const char* leftHand;
  const char* rightHand;

  // Practice
  const char* handLabel;
  const char* waitForInput;
  const char* assist;
  const char* assistOff;
  const char* assistSlow;
  const char* assistFast;
  const char* loop;
  const char* loopFrom;
  const char* loopTo;
  const char* autoSpeed;
  const char* accuracy;
  const char* combo;
  const char* hits;
  const char* wrongNotes;
  const char* skipped;
  const char* targetKeys;
  const char* resetStats;
  const char* practiceComplete;
  const char* waitingHint;
  const char* playHint;

  // Tracks
  const char* tracks;
  const char* muteTrack;

  // Settings
  const char* sectionMidiIn;
  const char* sectionMidiOut;
  const char* sectionAudio;
  const char* sectionDisplay;
  const char* sectionKeyboard;
  const char* inputDevice;
  const char* outputDevice;
  const char* refresh;
  const char* noDevice;
  const char* channel;
  const char* allChannels;
  const char* velocityCurve;
  const char* curveLinear;
  const char* curveSoft;
  const char* curveHard;
  const char* curveFixed;
  const char* transpose;
  const char* latency;
  const char* latencyHint;
  const char* masterVolume;
  const char* metronomeVolume;
  const char* audioEngine;
  const char* audioRunning;
  const char* audioStopped;
  const char* theme;
  const char* themeDark;
  const char* themeLight;
  const char* noteNames;
  const char* lookahead;
  const char* lookaheadHint;
  const char* language;
  const char* chinese;
  const char* english;
  const char* deviceHint;
  const char* noSongLoaded;
  const char* notesCount;
  const char* noneOption;
  const char* rangeLabel;
  const char* keyCount;
  const char* firstKey;
  const char* autoFollow;
  const char* alignHardware;
  const char* hardwareLow;
  const char* outOfRangeAccomp;
  const char* keyboardRangeHint;
  const char* shiftUp;
  const char* shiftDown;
  const char* windowLabel;
  const char* hardwareLabel;

  // Status
  const char* connected;
  const char* disconnected;
  const char* keyboardHint;
};

const Strings& strings(Lang lang);
Lang detectSystemLanguage();

} // namespace nr
