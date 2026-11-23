/**
 * NoteRain i18n messages.
 *
 * English is the source of truth: `MessageKey` is derived from the `en` table,
 * and the `zh` table is typed as `Record<MessageKey, string>`, so a missing
 * Chinese string is a TypeScript error rather than a silent English fallback.
 */

export type Language = 'en' | 'zh';

export const LANGUAGES: readonly Language[] = ['en', 'zh'] as const;

export const LANGUAGE_LABELS: Record<Language, string> = {
  en: 'English',
  zh: '简体中文',
};

/** Compact label used inside the toolbar chip. */
export const LANGUAGE_CHIPS: Record<Language, string> = {
  en: 'EN',
  zh: '中',
};

const en = {
  // --- Common -------------------------------------------------------------
  'common.on': 'On',
  'common.off': 'Off',
  'common.ok': 'OK',
  'common.cancel': 'Cancel',
  'common.language': 'Language',
  'common.switchToChinese': 'Switch to Chinese',
  'common.switchToEnglish': 'Switch to English',

  // --- App shell ----------------------------------------------------------
  'app.emptyTitle': 'Drop a MIDI file here',
  'app.emptyHint': 'or pick a file with “Open MIDI” above',
  'app.emptyDrop': 'Release to load the file',
  'app.emptyFormats': 'Supports .mid and .midi files',
  'app.emptySample': 'Try the sample song',
  'app.sampleName': 'Cantina Band',
  'app.loadingFile': 'Loading…',
  'app.shortcuts': 'Space play/pause · ← → seek 5s · ↑ ↓ speed · L loop · M mute',
  'app.desktopOnly': 'Desktop build',

  // --- Toolbar / file -----------------------------------------------------
  'controls.openMidi': 'Open MIDI',
  'controls.selectFile': 'Select file…',
  'controls.expand': 'Show controls',
  'controls.collapse': 'Hide controls',
  'controls.fileName': 'File',
  'controls.measure': 'M{current}/{total}',
  'controls.seek': 'Playback position',

  // --- Audio --------------------------------------------------------------
  'controls.muteAudio': 'Mute audio',
  'controls.unmuteAudio': 'Enable audio',
  'controls.volume': 'Volume: {percent}%',

  // --- Theme --------------------------------------------------------------
  'controls.themeToLight': 'Switch to light theme',
  'controls.themeToDark': 'Switch to dark theme',

  // --- View ---------------------------------------------------------------
  'controls.view': 'View',
  'controls.viewFalling': 'Falling notes',
  'controls.viewSheet': 'Sheet music',
  'controls.colors': 'Colors',
  'controls.colorByTrack': 'Track',
  'controls.colorByPitch': 'Pitch',
  'controls.colorByTrackTitle': 'Color notes by track',
  'controls.colorByPitchTitle': 'Color notes by pitch (C, D, E, …)',
  'controls.showNotesOverlay': 'Show notes / chords',
  'controls.display': 'Display',

  // --- Practice -----------------------------------------------------------
  'controls.practice': 'Practice',
  'controls.waitForInput': 'Wait for input',
  'controls.grace': 'Grace',
  'controls.graceTitle': 'Grace period for early note hits',
  'controls.speed': 'Speed',
  'controls.speedUnit': '%',
  'controls.loop': 'Loop',
  'controls.loopToggle': 'Toggle loop',
  'controls.loopStart': 'Loop start measure',
  'controls.loopEnd': 'Loop end measure',
  'controls.loopClear': 'Clear loop',
  'controls.fitKeyboard': 'Fit keyboard to song',

  // --- Sound / devices ----------------------------------------------------
  'controls.sound': 'Sound',
  'controls.playMidiInput': 'Play MIDI input',
  'controls.loadingSamples': 'Loading samples…',
  'controls.devices': 'Devices',
  'controls.midiInput': 'MIDI input',
  'controls.midiUnavailable': 'MIDI not available',
  'controls.noDevices': 'No devices',
  'controls.none': 'None',

  // --- Tracks -------------------------------------------------------------
  'controls.tracks': 'Tracks ({count})',
  'controls.trackShow': 'Render only',
  'controls.trackHide': 'Hide track',
  'controls.trackPlay': 'Play track audio',
  'controls.trackMute': 'Mute track',
  'controls.trackEnable': 'Enable track',
  'controls.trackName': 'Track',

  // --- Loading screen -----------------------------------------------------
  'loading.downloading': 'Downloading piano samples',
  'loading.decoding': 'Decoding piano samples',
  'loading.samples': '{progress} / {total} samples',
  'loading.layers': '{progress} / {total} velocity layers',
  'loading.hint': 'First launch only — samples are cached afterwards.',

  // --- Chord quality names (used by the notes overlay) --------------------
  'chord.major': 'Major',
  'chord.maj7': 'Major 7',
  'chord.7': 'Dominant 7',
  'chord.add9': 'Add 9',
  'chord.maj9': 'Major 9',
  'chord.6': 'Sixth',
  'chord.m': 'Minor',
  'chord.m7': 'Minor 7',
  'chord.mMaj7': 'Minor major 7',
  'chord.m9': 'Minor 9',
  'chord.m6': 'Minor 6',
  'chord.dim': 'Diminished',
  'chord.dim7': 'Diminished 7',
  'chord.m7b5': 'Half diminished',
  'chord.aug': 'Augmented',
  'chord.aug7': 'Augmented 7',
  'chord.sus4': 'Sus4',
  'chord.sus2': 'Sus2',
  'chord.7sus4': '7sus4',
  'chord.5': 'Power chord',
} as const;

export type MessageKey = keyof typeof en;

const zh: Record<MessageKey, string> = {
  // --- Common -------------------------------------------------------------
  'common.on': '开',
  'common.off': '关',
  'common.ok': '确定',
  'common.cancel': '取消',
  'common.language': '语言',
  'common.switchToChinese': '切换到中文',
  'common.switchToEnglish': '切换到英文',

  // --- App shell ----------------------------------------------------------
  'app.emptyTitle': '把 MIDI 文件拖到这里',
  'app.emptyHint': '或点击上方的“打开 MIDI”选择文件',
  'app.emptyDrop': '松开即可载入',
  'app.emptyFormats': '支持 .mid / .midi 格式',
  'app.emptySample': '试试示例曲目',
  'app.sampleName': 'Cantina Band',
  'app.loadingFile': '正在载入…',
  'app.shortcuts': '空格 播放/暂停 · ← → 快退/快进 5 秒 · ↑ ↓ 调速 · L 循环 · M 静音',
  'app.desktopOnly': '桌面版',

  // --- Toolbar / file -----------------------------------------------------
  'controls.openMidi': '打开 MIDI',
  'controls.selectFile': '选择文件…',
  'controls.expand': '展开控制面板',
  'controls.collapse': '收起控制面板',
  'controls.fileName': '文件',
  'controls.measure': '{current}/{total} 小节',
  'controls.seek': '播放进度',

  // --- Audio --------------------------------------------------------------
  'controls.muteAudio': '静音',
  'controls.unmuteAudio': '开启声音',
  'controls.volume': '音量：{percent}%',

  // --- Theme --------------------------------------------------------------
  'controls.themeToLight': '切换到浅色主题',
  'controls.themeToDark': '切换到深色主题',

  // --- View ---------------------------------------------------------------
  'controls.view': '视图',
  'controls.viewFalling': '落键瀑布',
  'controls.viewSheet': '五线谱',
  'controls.colors': '配色',
  'controls.colorByTrack': '按音轨',
  'controls.colorByPitch': '按音高',
  'controls.colorByTrackTitle': '按音轨为音符上色',
  'controls.colorByPitchTitle': '按音高为音符上色（C、D、E…）',
  'controls.showNotesOverlay': '显示音符 / 和弦',
  'controls.display': '显示',

  // --- Practice -----------------------------------------------------------
  'controls.practice': '练习',
  'controls.waitForInput': '等待按键',
  'controls.grace': '容差',
  'controls.graceTitle': '提前按下琴键的容差时间',
  'controls.speed': '速度',
  'controls.speedUnit': '%',
  'controls.loop': '循环',
  'controls.loopToggle': '开启 / 关闭循环',
  'controls.loopStart': '循环起始小节',
  'controls.loopEnd': '循环结束小节',
  'controls.loopClear': '清除循环',
  'controls.fitKeyboard': '键盘适配曲目音域',

  // --- Sound / devices ----------------------------------------------------
  'controls.sound': '声音',
  'controls.playMidiInput': '键盘实时发声',
  'controls.loadingSamples': '正在加载音色…',
  'controls.devices': '设备',
  'controls.midiInput': 'MIDI 输入',
  'controls.midiUnavailable': 'MIDI 不可用',
  'controls.noDevices': '未检测到设备',
  'controls.none': '无',

  // --- Tracks -------------------------------------------------------------
  'controls.tracks': '音轨（{count}）',
  'controls.trackShow': '仅显示（不发声）',
  'controls.trackHide': '隐藏音轨',
  'controls.trackPlay': '播放该音轨',
  'controls.trackMute': '静音该音轨',
  'controls.trackEnable': '启用音轨',
  'controls.trackName': '音轨',

  // --- Loading screen -----------------------------------------------------
  'loading.downloading': '正在下载钢琴音色',
  'loading.decoding': '正在解码钢琴音色',
  'loading.samples': '已下载 {progress} / {total} 个采样',
  'loading.layers': '已解码 {progress} / {total} 个力度层',
  'loading.hint': '仅在首次启动时下载，之后会使用本地缓存。',

  // --- Chord quality names ------------------------------------------------
  'chord.major': '大三和弦',
  'chord.maj7': '大七和弦',
  'chord.7': '属七和弦',
  'chord.add9': '加九和弦',
  'chord.maj9': '大九和弦',
  'chord.6': '六和弦',
  'chord.m': '小三和弦',
  'chord.m7': '小七和弦',
  'chord.mMaj7': '小大七和弦',
  'chord.m9': '小九和弦',
  'chord.m6': '小六和弦',
  'chord.dim': '减三和弦',
  'chord.dim7': '减七和弦',
  'chord.m7b5': '半减七和弦',
  'chord.aug': '增三和弦',
  'chord.aug7': '增七和弦',
  'chord.sus4': '挂四和弦',
  'chord.sus2': '挂二和弦',
  'chord.7sus4': '七挂四和弦',
  'chord.5': '强力和弦',
};

export const MESSAGES: Record<Language, Record<MessageKey, string>> = { en, zh };

/** BCP-47 tag used for the document `lang` attribute. */
export const HTML_LANG: Record<Language, string> = {
  en: 'en',
  zh: 'zh-CN',
};

/** Guess the best language for a first launch. */
export function detectLanguage(): Language {
  try {
    const candidates =
      typeof navigator !== 'undefined' && navigator.languages?.length
        ? navigator.languages
        : typeof navigator !== 'undefined'
          ? [navigator.language]
          : [];
    for (const candidate of candidates) {
      const value = (candidate || '').toLowerCase();
      if (value.startsWith('zh')) return 'zh';
      if (value.startsWith('en')) return 'en';
    }
  } catch {
    // navigator is unavailable (SSR / tests) — fall through
  }
  return 'en';
}

/** Translate a key, interpolating `{placeholder}` tokens. */
export function translate(
  language: Language,
  key: MessageKey,
  params?: Record<string, string | number>,
): string {
  const table = MESSAGES[language] ?? MESSAGES.en;
  let text = table[key] ?? MESSAGES.en[key] ?? key;
  if (params) {
    for (const [name, value] of Object.entries(params)) {
      text = text.split(`{${name}}`).join(String(value));
    }
  }
  return text;
}

/** Look up a chord quality name by its short suffix (`''` = major triad). */
export function chordQualityName(
  language: Language,
  suffix: string,
): string {
  const base = suffix.split('/')[0];
  const key = `chord.${base || 'major'}` as MessageKey;
  const table = MESSAGES[language] ?? MESSAGES.en;
  return table[key] ?? base;
}
