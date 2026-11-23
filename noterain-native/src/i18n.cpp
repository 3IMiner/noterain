#include "i18n.h"

#include <windows.h>

namespace nr {
namespace {

Strings makeZh() {
  Strings s{};
  s.appName = "NoteRain 雨音";
  s.openMidi = "打开 MIDI";
  s.openDialogTitle = "打开 MIDI 文件";
  s.demoTitle = "示例练习曲";
  s.noFileTitle = "还没有载入曲目";
  s.noFileBody = "把 .mid 文件拖进窗口，或者点下面的按钮";
  s.formatsHint = "支持 .mid / .midi，1 秒内即可载入";
  s.shortcutHint = "空格 播放/暂停 · ←/→ 跳转 · ↑/↓ 速度 · L 循环 · M 节拍器 · W 练习模式 · [ ] 移窗";
  s.settings = "设置";
  s.back = "返回";
  s.close = "关闭";
  s.dropHint = "松开即可载入";
  s.play = "播放";
  s.pause = "暂停";
  s.stop = "停止";
  s.speed = "速度";
  s.metronome = "节拍器";
  s.listen = "听";
  s.practice = "练";
  s.bar = "小节";
  s.bothHands = "双手";
  s.leftHand = "左手";
  s.rightHand = "右手";
  s.handLabel = "练习手别";
  s.waitForInput = "等待按键";
  s.assist = "卡住时";
  s.assistOff = "一直等";
  s.assistSlow = "8 秒后跳过";
  s.assistFast = "4 秒后跳过";
  s.loop = "循环";
  s.loopFrom = "起";
  s.loopTo = "止";
  s.autoSpeed = "循环后提速";
  s.accuracy = "准确率";
  s.combo = "连击";
  s.hits = "命中";
  s.wrongNotes = "错音";
  s.skipped = "跳过";
  s.targetKeys = "待弹音符";
  s.resetStats = "重置统计";
  s.practiceComplete = "这一段练完了！";
  s.waitingHint = "请按亮起的琴键";
  s.playHint = "跟着落下的音符弹奏";
  s.tracks = "音轨";
  s.muteTrack = "启用/静音";
  s.sectionMidiIn = "MIDI 输入";
  s.sectionMidiOut = "MIDI 输出";
  s.sectionAudio = "音频";
  s.sectionDisplay = "显示";
  s.sectionKeyboard = "键盘音域";
  s.inputDevice = "输入设备";
  s.outputDevice = "输出设备";
  s.refresh = "刷新设备";
  s.noDevice = "未检测到设备";
  s.channel = "接收通道";
  s.allChannels = "全部";
  s.velocityCurve = "力度曲线";
  s.curveLinear = "线性";
  s.curveSoft = "柔和";
  s.curveHard = "硬朗";
  s.curveFixed = "固定力度";
  s.transpose = "移调";
  s.latency = "延迟补偿";
  s.latencyHint = "键盘声卡延迟会导致判定偏早/偏晚，可在此微调";
  s.masterVolume = "主音量";
  s.metronomeVolume = "节拍音量";
  s.audioEngine = "音频引擎";
  s.audioRunning = "运行中";
  s.audioStopped = "未启动";
  s.theme = "主题";
  s.themeDark = "深色";
  s.themeLight = "浅色";
  s.noteNames = "琴键显示音名";
  s.lookahead = "下落预读";
  s.lookaheadHint = "音符提前多久出现，越大越容易预判";
  s.language = "界面语言";
  s.chinese = "简体中文";
  s.english = "English";
  s.deviceHint = "热插拔后请点「刷新设备」";
  s.noSongLoaded = "未载入曲目";
  s.notesCount = "个音符";
  s.noneOption = "（不使用）";
  s.rangeLabel = "音域";
  s.keyCount = "键数";
  s.firstKey = "起始音";
  s.autoFollow = "窗口跟随乐句";
  s.alignHardware = "键位对齐";
  s.hardwareLow = "硬件最低音";
  s.outOfRangeAccomp = "音域外自动伴奏";
  s.keyboardRangeHint = "32/37 键键盘：开启「键位对齐」，物理键始终对应当前可见音域；窗口会随乐句自动移动。";
  s.shiftUp = "升八度";
  s.shiftDown = "降八度";
  s.windowLabel = "窗口";
  s.hardwareLabel = "硬件";
  s.connected = "已连接";
  s.disconnected = "未连接";
  s.keyboardHint = "也可以直接用鼠标点琴键练习";
  return s;
}

Strings makeEn() {
  Strings s{};
  s.appName = "NoteRain";
  s.openMidi = "Open MIDI";
  s.openDialogTitle = "Open MIDI file";
  s.demoTitle = "Demo study";
  s.noFileTitle = "No song loaded yet";
  s.noFileBody = "Drop a .mid file onto the window, or use the buttons below";
  s.formatsHint = "Accepts .mid / .midi — loads in under a second";
  s.shortcutHint = "Space play/pause · ←/→ seek · ↑/↓ speed · L loop · M metronome · W practice · [ ] shift window";
  s.settings = "Settings";
  s.back = "Back";
  s.close = "Close";
  s.dropHint = "Release to load";
  s.play = "Play";
  s.pause = "Pause";
  s.stop = "Stop";
  s.speed = "Speed";
  s.metronome = "Metronome";
  s.listen = "Listen";
  s.practice = "Practice";
  s.bar = "bar";
  s.bothHands = "Both";
  s.leftHand = "Left";
  s.rightHand = "Right";
  s.handLabel = "Practice hand";
  s.waitForInput = "Wait for input";
  s.assist = "When stuck";
  s.assistOff = "Wait forever";
  s.assistSlow = "Skip after 8s";
  s.assistFast = "Skip after 4s";
  s.loop = "Loop";
  s.loopFrom = "From";
  s.loopTo = "To";
  s.autoSpeed = "Speed up after loop";
  s.accuracy = "Accuracy";
  s.combo = "Combo";
  s.hits = "Hits";
  s.wrongNotes = "Wrong";
  s.skipped = "Skipped";
  s.targetKeys = "Next notes";
  s.resetStats = "Reset stats";
  s.practiceComplete = "Section complete!";
  s.waitingHint = "Play the lit keys";
  s.playHint = "Play along with the falling notes";
  s.tracks = "Tracks";
  s.muteTrack = "Enable / mute";
  s.sectionMidiIn = "MIDI input";
  s.sectionMidiOut = "MIDI output";
  s.sectionAudio = "Audio";
  s.sectionDisplay = "Display";
  s.sectionKeyboard = "Keyboard range";
  s.inputDevice = "Input device";
  s.outputDevice = "Output device";
  s.refresh = "Refresh devices";
  s.noDevice = "No devices found";
  s.channel = "Channel";
  s.allChannels = "All";
  s.velocityCurve = "Velocity curve";
  s.curveLinear = "Linear";
  s.curveSoft = "Soft";
  s.curveHard = "Hard";
  s.curveFixed = "Fixed";
  s.transpose = "Transpose";
  s.latency = "Latency offset";
  s.latencyHint = "Compensates for keyboard/audio latency in scoring";
  s.masterVolume = "Master volume";
  s.metronomeVolume = "Metronome volume";
  s.audioEngine = "Audio engine";
  s.audioRunning = "running";
  s.audioStopped = "stopped";
  s.theme = "Theme";
  s.themeDark = "Dark";
  s.themeLight = "Light";
  s.noteNames = "Note names on keys";
  s.lookahead = "Fall length";
  s.lookaheadHint = "How far ahead notes appear";
  s.language = "Language";
  s.chinese = "简体中文";
  s.english = "English";
  s.deviceHint = "Hot-plugged a device? Press Refresh";
  s.noSongLoaded = "No song loaded";
  s.notesCount = "notes";
  s.noneOption = "(none)";
  s.rangeLabel = "Range";
  s.keyCount = "Key count";
  s.firstKey = "First key";
  s.autoFollow = "Follow the music";
  s.alignHardware = "Align hardware keys";
  s.hardwareLow = "Hardware low key";
  s.outOfRangeAccomp = "Auto-play out of range";
  s.keyboardRangeHint =
      "For 25-37 key controllers: turn on \"Align hardware keys\" so the physical keys "
      "always match the visible window; the window follows the music automatically.";
  s.shiftUp = "Octave up";
  s.shiftDown = "Octave down";
  s.windowLabel = "Window";
  s.hardwareLabel = "Hardware";
  s.connected = "connected";
  s.disconnected = "disconnected";
  s.keyboardHint = "You can also click the on-screen keys to practise";
  return s;
}

} // namespace

const Strings& strings(Lang lang) {
  static const Strings zh = makeZh();
  static const Strings en = makeEn();
  return lang == Lang::En ? en : zh;
}

Lang detectSystemLanguage() {
  const LANGID id = GetUserDefaultUILanguage();
  const WORD primary = PRIMARYLANGID(id);
  return primary == LANG_CHINESE ? Lang::Zh : Lang::En;
}

} // namespace nr
