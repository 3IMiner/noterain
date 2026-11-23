import { useEffect, useRef, useState } from 'react';
import {
  Play,
  Pause,
  Square,
  Volume2,
  VolumeX,
  Sun,
  Moon,
  ChevronUp,
  ChevronDown,
  Repeat,
  X,
  Eye,
  EyeOff,
  FolderOpen,
  Languages,
  Piano,
  Music,
} from 'lucide-react';
import { usePlayback } from '../../hooks/usePlayback';
import { useMidiFile } from '../../hooks/useMidiFile';
import { useMidiInput } from '../../hooks/useMidiInput';
import { useI18n } from '../../i18n';
import {
  useMidiStore,
  getMeasureCount,
  getSecondsPerMeasure,
} from '../../stores/midiStore';
import styles from './Controls.module.css';

interface ControlsProps {
  isLoadingFullVelocity: boolean;
}

interface SegmentOption<T extends string> {
  value: T;
  label: string;
  icon?: React.ReactNode;
}

/** Pill-style radio group used for view/color pickers. */
function Segmented<T extends string>({
  value,
  options,
  onChange,
}: {
  value: T;
  options: SegmentOption<T>[];
  onChange: (value: T) => void;
}) {
  return (
    <div className={styles.segmented} role="group">
      {options.map((option) => (
        <button
          key={option.value}
          type="button"
          className={`${styles.segment} ${
            option.value === value ? styles.segmentActive : ''
          }`}
          aria-pressed={option.value === value}
          onClick={() => onChange(option.value)}
        >
          {option.icon}
          <span>{option.label}</span>
        </button>
      ))}
    </div>
  );
}

/** Label + switch combo. */
function Toggle({
  checked,
  onChange,
  label,
  title,
}: {
  checked: boolean;
  onChange: () => void;
  label: string;
  title?: string;
}) {
  return (
    <label className={styles.toggle} title={title}>
      <input type="checkbox" checked={checked} onChange={onChange} />
      <span className={styles.toggleTrack} aria-hidden="true">
        <span className={styles.toggleThumb} />
      </span>
      <span className={styles.toggleLabel}>{label}</span>
    </label>
  );
}

export function Controls({ isLoadingFullVelocity }: ControlsProps) {
  const fileInputRef = useRef<HTMLInputElement>(null);
  const tracksRef = useRef<HTMLDivElement>(null);
  const [isExpanded, setIsExpanded] = useState(true);
  const [tracksDropdownOpen, setTracksDropdownOpen] = useState(false);

  const { t, language, toggleLanguage } = useI18n();

  const {
    isPlaying,
    currentTime,
    speed,
    waitMode,
    loopEnabled,
    loopStartMeasure,
    loopEndMeasure,
    togglePlay,
    stop,
    seekToPercent,
    setSpeed,
    toggleWaitMode,
    getProgress,
    setLoopRange,
    toggleLoop,
    clearLoop,
  } = usePlayback();

  const { files, currentFile, currentFileId, setCurrentFile, handleFileInput } =
    useMidiFile();

  const { inputs, selectedInput, selectInput, isEnabled } = useMidiInput();

  const { settings, updateSettings } = useMidiStore();

  // Auto-collapse when playback starts
  useEffect(() => {
    if (isPlaying) {
      setIsExpanded(false);
    }
  }, [isPlaying]);

  // Close the tracks dropdown on an outside click
  useEffect(() => {
    if (!tracksDropdownOpen) return;
    const handlePointerDown = (event: MouseEvent) => {
      if (!tracksRef.current?.contains(event.target as Node)) {
        setTracksDropdownOpen(false);
      }
    };
    document.addEventListener('mousedown', handlePointerDown);
    return () => document.removeEventListener('mousedown', handlePointerDown);
  }, [tracksDropdownOpen]);

  const formatTime = (seconds: number): string => {
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
  };

  const truncateFilename = (name: string, maxLength = 25): string => {
    if (name.length <= maxLength) return name;
    const ext =
      name.lastIndexOf('.') > 0 ? name.slice(name.lastIndexOf('.')) : '';
    const baseName = name.slice(0, name.length - ext.length);
    const truncatedLength = maxLength - ext.length - 3;
    return baseName.slice(0, truncatedLength) + '...' + ext;
  };

  const currentMeasure =
    currentFile && getSecondsPerMeasure(currentFile) > 0
      ? Math.floor(currentTime / getSecondsPerMeasure(currentFile)) + 1
      : 0;

  return (
    <div className={styles.controls} data-expanded={isExpanded}>
      {/* ---- Header row (always visible) ---- */}
      <div className={styles.header}>
        <div className={styles.headerLeft}>
          <div className={styles.brand} title="NoteRain">
            <img src="/logo.png" alt="" className={styles.logo} />
            <span className={styles.brandName}>NoteRain</span>
          </div>

          <button
            className={`${styles.button} ${styles.primary}`}
            onClick={() => fileInputRef.current?.click()}
            title={t('controls.openMidi')}
          >
            <FolderOpen size={15} />
            <span>{t('controls.openMidi')}</span>
          </button>
          <input
            ref={fileInputRef}
            type="file"
            accept=".mid,.midi"
            onChange={handleFileInput}
            style={{ display: 'none' }}
          />

          {files.length > 0 && (
            <select
              className={styles.select}
              value={currentFileId || ''}
              onChange={(e) => setCurrentFile(e.target.value || null)}
              title={t('controls.fileName')}
            >
              <option value="">{t('controls.selectFile')}</option>
              {files.map((file) => (
                <option key={file.id} value={file.id} title={file.name}>
                  {truncateFilename(file.name)}
                </option>
              ))}
            </select>
          )}
        </div>

        {/* Playback controls - centered */}
        <div className={styles.headerCenter}>
          <div className={styles.transport}>
            <button
              className={`${styles.transportButton} ${styles.playButton}`}
              onClick={togglePlay}
              disabled={!currentFile}
              title={isPlaying ? 'Pause' : 'Play'}
              aria-label={isPlaying ? 'Pause' : 'Play'}
            >
              {isPlaying ? <Pause size={16} /> : <Play size={16} />}
            </button>

            <button
              className={styles.transportButton}
              onClick={stop}
              disabled={!currentFile}
              title="Stop"
              aria-label="Stop"
            >
              <Square size={14} />
            </button>
          </div>

          <div className={styles.timeDisplay}>
            <span className={styles.time}>
              {formatTime(currentTime)}
              <span className={styles.timeTotal}>
                {' / '}
                {formatTime(currentFile?.duration || 0)}
              </span>
            </span>
            {currentFile && (
              <span className={styles.measureDisplay}>
                {t('controls.measure', {
                  current: currentMeasure,
                  total: getMeasureCount(currentFile),
                })}
              </span>
            )}
          </div>

          <input
            type="range"
            className={styles.progressBar}
            min="0"
            max="100"
            value={getProgress() * 100}
            onChange={(e) => seekToPercent(parseFloat(e.target.value) / 100)}
            disabled={!currentFile}
            title={t('controls.seek')}
            aria-label={t('controls.seek')}
          />
        </div>

        <div className={styles.headerRight}>
          {/* Volume control */}
          <div className={styles.volumeControl}>
            <button
              className={`${styles.iconButton} ${!settings.audioEnabled ? styles.mutedIcon : ''}`}
              onClick={() =>
                updateSettings({ audioEnabled: !settings.audioEnabled })
              }
              title={
                settings.audioEnabled
                  ? t('controls.muteAudio')
                  : t('controls.unmuteAudio')
              }
              aria-label={
                settings.audioEnabled
                  ? t('controls.muteAudio')
                  : t('controls.unmuteAudio')
              }
            >
              {settings.audioEnabled ? (
                <Volume2 size={16} />
              ) : (
                <VolumeX size={16} />
              )}
            </button>
            {settings.audioEnabled && (
              <input
                type="range"
                className={styles.volumeSlider}
                min="0"
                max="100"
                value={settings.volume * 100}
                onChange={(e) =>
                  updateSettings({ volume: parseFloat(e.target.value) / 100 })
                }
                title={t('controls.volume', {
                  percent: Math.round(settings.volume * 100),
                })}
                aria-label={t('controls.volume', {
                  percent: Math.round(settings.volume * 100),
                })}
              />
            )}
          </div>

          {/* Language toggle */}
          <button
            className={styles.chip}
            onClick={toggleLanguage}
            title={
              language === 'zh'
                ? t('common.switchToEnglish')
                : t('common.switchToChinese')
            }
            aria-label={t('common.language')}
          >
            <Languages size={14} />
            <span>{language === 'zh' ? '中' : 'EN'}</span>
          </button>

          {/* Theme toggle */}
          <button
            className={styles.iconButton}
            onClick={() =>
              updateSettings({
                theme: settings.theme === 'mocha' ? 'latte' : 'mocha',
              })
            }
            title={
              settings.theme === 'mocha'
                ? t('controls.themeToLight')
                : t('controls.themeToDark')
            }
            aria-label={
              settings.theme === 'mocha'
                ? t('controls.themeToLight')
                : t('controls.themeToDark')
            }
          >
            {settings.theme === 'mocha' ? (
              <Sun size={16} />
            ) : (
              <Moon size={16} />
            )}
          </button>

          {/* Toggle expand button */}
          <button
            className={styles.iconButton}
            onClick={() => setIsExpanded(!isExpanded)}
            title={isExpanded ? t('controls.collapse') : t('controls.expand')}
            aria-label={isExpanded ? t('controls.collapse') : t('controls.expand')}
            aria-expanded={isExpanded}
          >
            {isExpanded ? <ChevronUp size={16} /> : <ChevronDown size={16} />}
          </button>
        </div>
      </div>

      {/* ---- Expanded panel ---- */}
      {isExpanded && (
        <div className={styles.panel}>
          {/* View + colors */}
          <div className={styles.group}>
            <span className={styles.groupLabel}>{t('controls.view')}</span>
            <Segmented
              value={settings.showSheetMusic ? 'sheet' : 'falling'}
              onChange={(value) =>
                updateSettings({ showSheetMusic: value === 'sheet' })
              }
              options={[
                {
                  value: 'falling',
                  label: t('controls.viewFalling'),
                  icon: <Piano size={14} />,
                },
                {
                  value: 'sheet',
                  label: t('controls.viewSheet'),
                  icon: <Music size={14} />,
                },
              ]}
            />
            {!settings.showSheetMusic && (
              <>
                <span className={styles.groupDivider} />
                <span className={styles.groupLabel}>{t('controls.colors')}</span>
                <Segmented
                  value={settings.noteColorMode}
                  onChange={(value) =>
                    updateSettings({ noteColorMode: value as 'track' | 'pitch' })
                  }
                  options={[
                    {
                      value: 'track' as const,
                      label: t('controls.colorByTrack'),
                    },
                    {
                      value: 'pitch' as const,
                      label: t('controls.colorByPitch'),
                    },
                  ]}
                />
              </>
            )}
          </div>

          {/* Practice */}
          <div className={styles.group}>
            <span className={styles.groupLabel}>{t('controls.practice')}</span>

            <Toggle
              checked={waitMode}
              onChange={toggleWaitMode}
              label={t('controls.waitForInput')}
            />

            {waitMode && (
              <label className={styles.inlineField}>
                <span className={styles.fieldLabel}>{t('controls.grace')}</span>
                <select
                  className={styles.selectSmall}
                  value={settings.waitModeGracePeriod}
                  onChange={(e) =>
                    updateSettings({
                      waitModeGracePeriod: parseInt(e.target.value),
                    })
                  }
                  title={t('controls.graceTitle')}
                >
                  {[0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500].map(
                    (ms) => (
                      <option key={ms} value={ms}>
                        {ms}ms
                      </option>
                    ),
                  )}
                </select>
              </label>
            )}

            <span className={styles.groupDivider} />

            <label className={styles.inlineField}>
              <span className={styles.fieldLabel}>{t('controls.speed')}</span>
              <input
                type="number"
                className={styles.numberInput}
                min="10"
                max="200"
                step="5"
                value={Math.round(speed * 100)}
                onChange={(e) => {
                  const val = parseInt(e.target.value);
                  if (!isNaN(val)) {
                    setSpeed(Math.max(10, Math.min(200, val)) / 100);
                  }
                }}
              />
              <span className={styles.fieldLabel}>{t('controls.speedUnit')}</span>
            </label>

            {currentFile && (
              <>
                <span className={styles.groupDivider} />
                <span className={styles.fieldLabel}>{t('controls.loop')}</span>
                <button
                  className={`${styles.iconSmall} ${loopEnabled ? styles.active : ''}`}
                  onClick={toggleLoop}
                  title={t('controls.loopToggle')}
                  aria-label={t('controls.loopToggle')}
                >
                  <Repeat size={14} />
                </button>
                {loopEnabled && (
                  <>
                    <input
                      type="number"
                      className={styles.measureInput}
                      min="1"
                      max={getMeasureCount(currentFile)}
                      value={(loopStartMeasure ?? 0) + 1}
                      onChange={(e) => {
                        const val = parseInt(e.target.value) - 1;
                        if (!isNaN(val)) {
                          setLoopRange(Math.max(0, val), loopEndMeasure);
                        }
                      }}
                      title={t('controls.loopStart')}
                      aria-label={t('controls.loopStart')}
                    />
                    <span className={styles.loopSeparator}>–</span>
                    <input
                      type="number"
                      className={styles.measureInput}
                      min="1"
                      max={getMeasureCount(currentFile)}
                      value={(loopEndMeasure ?? 0) + 1}
                      onChange={(e) => {
                        const val = parseInt(e.target.value) - 1;
                        if (!isNaN(val)) {
                          const maxMeasure = getMeasureCount(currentFile) - 1;
                          setLoopRange(
                            loopStartMeasure,
                            Math.min(val, maxMeasure),
                          );
                        }
                      }}
                      title={t('controls.loopEnd')}
                      aria-label={t('controls.loopEnd')}
                    />
                    <button
                      className={styles.iconSmall}
                      onClick={clearLoop}
                      title={t('controls.loopClear')}
                      aria-label={t('controls.loopClear')}
                    >
                      <X size={13} />
                    </button>
                  </>
                )}
              </>
            )}
          </div>

          {/* Display */}
          <div className={styles.group}>
            <span className={styles.groupLabel}>{t('controls.display')}</span>
            <Toggle
              checked={settings.showNotesOverlay}
              onChange={() =>
                updateSettings({
                  showNotesOverlay: !settings.showNotesOverlay,
                })
              }
              label={t('controls.showNotesOverlay')}
            />
            <Toggle
              checked={settings.fitKeyboardToSong}
              onChange={() =>
                updateSettings({
                  fitKeyboardToSong: !settings.fitKeyboardToSong,
                })
              }
              label={t('controls.fitKeyboard')}
            />
          </div>

          {/* Sound */}
          <div className={styles.group}>
            <span className={styles.groupLabel}>{t('controls.sound')}</span>
            <Toggle
              checked={settings.playMidiInputAudio}
              onChange={() =>
                updateSettings({
                  playMidiInputAudio: !settings.playMidiInputAudio,
                })
              }
              label={t('controls.playMidiInput')}
            />
            {isLoadingFullVelocity && (
              <span className={styles.loadingIndicator}>
                <span className={styles.spinner} />
                {t('controls.loadingSamples')}
              </span>
            )}
          </div>

          {/* MIDI devices */}
          <div className={styles.group}>
            <span className={styles.groupLabel}>{t('controls.devices')}</span>
            {!isEnabled ? (
              <span className={styles.warning}>
                {t('controls.midiUnavailable')}
              </span>
            ) : inputs.length === 0 ? (
              <span className={styles.muted}>{t('controls.noDevices')}</span>
            ) : (
              <select
                className={styles.selectSmall}
                value={selectedInput?.id || ''}
                onChange={(e) => selectInput(e.target.value || null)}
                title={t('controls.midiInput')}
              >
                <option value="">{t('controls.none')}</option>
                {inputs.map((device) => (
                  <option key={device.id} value={device.id}>
                    {device.name}
                  </option>
                ))}
              </select>
            )}
          </div>

          {/* Track list - inline for 2 or fewer, dropdown for more */}
          {currentFile && currentFile.tracks.length > 0 && (
            <div
              className={`${styles.group} ${styles.tracksGroup}`}
              ref={tracksRef}
            >
              <span className={styles.groupLabel}>
                {t('controls.tracks', { count: currentFile.tracks.length })}
              </span>

              {currentFile.tracks.length <= 2 &&
                currentFile.tracks.map((track) => (
                  <div
                    key={track.index}
                    className={styles.trackRow}
                    style={{ borderLeftColor: track.color }}
                  >
                    <input
                      type="checkbox"
                      checked={track.enabled}
                      onChange={() =>
                        useMidiStore
                          .getState()
                          .toggleTrack(currentFile.id, track.index)
                      }
                      title={t('controls.trackEnable')}
                    />
                    <span className={styles.trackName}>{track.name}</span>
                    <button
                      className={`${styles.trackButton} ${track.renderOnly || track.enabled ? styles.active : styles.crossed}`}
                      onClick={() =>
                        useMidiStore
                          .getState()
                          .toggleTrackRenderOnly(currentFile.id, track.index)
                      }
                      title={
                        track.renderOnly
                          ? t('controls.trackHide')
                          : t('controls.trackShow')
                      }
                      aria-label={
                        track.renderOnly
                          ? t('controls.trackHide')
                          : t('controls.trackShow')
                      }
                      disabled={track.enabled}
                    >
                      {track.renderOnly || track.enabled ? (
                        <Eye size={14} />
                      ) : (
                        <EyeOff size={14} />
                      )}
                    </button>
                    <button
                      className={`${styles.trackButton} ${track.playAudio ? styles.active : styles.crossed}`}
                      onClick={() =>
                        useMidiStore
                          .getState()
                          .toggleTrackPlayAudio(currentFile.id, track.index)
                      }
                      title={
                        track.playAudio
                          ? t('controls.trackMute')
                          : t('controls.trackPlay')
                      }
                      aria-label={
                        track.playAudio
                          ? t('controls.trackMute')
                          : t('controls.trackPlay')
                      }
                    >
                      {track.playAudio ? (
                        <Volume2 size={14} />
                      ) : (
                        <VolumeX size={14} />
                      )}
                    </button>
                  </div>
                ))}

              {currentFile.tracks.length > 2 && (
                <div className={styles.tracksDropdownWrap}>
                  <button
                    className={`${styles.button} ${styles.small} ${
                      tracksDropdownOpen ? styles.active : ''
                    }`}
                    onClick={() => setTracksDropdownOpen(!tracksDropdownOpen)}
                    aria-expanded={tracksDropdownOpen}
                  >
                    <span>
                      {t('controls.tracks', {
                        count: currentFile.tracks.length,
                      })}
                    </span>
                    {tracksDropdownOpen ? (
                      <ChevronUp size={14} />
                    ) : (
                      <ChevronDown size={14} />
                    )}
                  </button>
                  {tracksDropdownOpen && (
                    <div className={styles.tracksDropdown}>
                      {currentFile.tracks.map((track) => (
                        <div
                          key={track.index}
                          className={styles.trackItem}
                          style={{ borderLeftColor: track.color }}
                        >
                          <input
                            type="checkbox"
                            checked={track.enabled}
                            onChange={() =>
                              useMidiStore
                                .getState()
                                .toggleTrack(currentFile.id, track.index)
                            }
                          />
                          <span className={styles.trackName}>{track.name}</span>
                          <button
                            className={`${styles.trackButton} ${track.renderOnly || track.enabled ? styles.active : styles.crossed}`}
                            onClick={() =>
                              useMidiStore
                                .getState()
                                .toggleTrackRenderOnly(
                                  currentFile.id,
                                  track.index,
                                )
                            }
                            title={
                              track.renderOnly
                                ? t('controls.trackHide')
                                : t('controls.trackShow')
                            }
                            disabled={track.enabled}
                          >
                            {track.renderOnly || track.enabled ? (
                              <Eye size={14} />
                            ) : (
                              <EyeOff size={14} />
                            )}
                          </button>
                          <button
                            className={`${styles.trackButton} ${track.playAudio ? styles.active : styles.crossed}`}
                            onClick={() =>
                              useMidiStore
                                .getState()
                                .toggleTrackPlayAudio(
                                  currentFile.id,
                                  track.index,
                                )
                            }
                            title={
                              track.playAudio
                                ? t('controls.trackMute')
                                : t('controls.trackPlay')
                            }
                          >
                            {track.playAudio ? (
                              <Volume2 size={14} />
                            ) : (
                              <VolumeX size={14} />
                            )}
                          </button>
                        </div>
                      ))}
                    </div>
                  )}
                </div>
              )}
            </div>
          )}
        </div>
      )}
    </div>
  );
}
