import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { FileMusic, FolderOpen, Music } from 'lucide-react';
import { Controls } from './components/Controls/Controls';
import { FallingNotes } from './components/FallingNotes/FallingNotes';
import { LoadingScreen } from './components/LoadingScreen/LoadingScreen';
import { NotesOverlay } from './components/NotesOverlay/NotesOverlay';
import { SheetMusic } from './components/SheetMusic/SheetMusic';
import { PianoKeyboard } from './components/PianoKeyboard/PianoKeyboard';
import { useAudioEngine } from './hooks/useAudioEngine';
import { useMidiFile } from './hooks/useMidiFile';
import { useMidiInput } from './hooks/useMidiInput';
import { useKeyboardShortcuts } from './hooks/useKeyboardShortcuts';
import { HTML_LANG, useI18n } from './i18n';
import { getDesktopApi } from './lib/desktop';
import { useMidiStore, calculateNoteRange } from './stores/midiStore';
import './App.css';

/** Bundled demo song (see `client/public/midi/`). */
const SAMPLE_SONG_URL = '/midi/sample.mid';
const SAMPLE_SONG_NAME = 'Cantina Band.mid';

function App() {
  const { t, language } = useI18n();
  const {
    noteOn,
    noteOff,
    setSustain,
    resumeAudio,
    isLoading,
    loadingPhase,
    downloadProgress,
    decodeProgress,
    totalSamples,
    totalLayers,
    isLoadingFullVelocity,
  } = useAudioEngine();
  const { handleDrop, handleFileInput, loadFile, loadFromUrl, currentFile } =
    useMidiFile();
  const { settings, setLiveNote, addSatisfiedWaitNote } = useMidiStore();

  const emptyFileInputRef = useRef<HTMLInputElement>(null);
  const [isDragging, setIsDragging] = useState(false);
  const [isLoadingSample, setIsLoadingSample] = useState(false);
  const dragDepth = useRef(0);

  // Store audio functions in ref to avoid MIDI listener re-subscriptions
  const audioRef = useRef({
    noteOn,
    noteOff,
    setSustain,
    resumeAudio,
    settings,
    isLoading,
  });
  audioRef.current = {
    noteOn,
    noteOff,
    setSustain,
    resumeAudio,
    settings,
    isLoading,
  };

  // MIDI input callbacks - play audio when hardware MIDI notes are received
  const handleMidiNoteOn = useCallback(
    async (noteNumber: number, velocity: number) => {
      const { noteOn, resumeAudio, settings, isLoading } = audioRef.current;
      if (isLoading) return; // Block input while loading
      if (settings.playMidiInputAudio) {
        await resumeAudio();
        noteOn(noteNumber, velocity);
      }
    },
    [],
  );

  const handleMidiNoteOff = useCallback((noteNumber: number) => {
    const { noteOff, settings, isLoading } = audioRef.current;
    if (isLoading) return; // Block input while loading
    if (settings.playMidiInputAudio) {
      noteOff(noteNumber);
    }
  }, []);

  const handleMidiSustain = useCallback((isPressed: boolean) => {
    const { setSustain, settings, isLoading } = audioRef.current;
    if (isLoading) return; // Block input while loading
    if (settings.playMidiInputAudio) {
      setSustain(isPressed);
    }
  }, []);

  // Initialize MIDI input with audio callbacks
  useMidiInput({
    onNoteOn: handleMidiNoteOn,
    onNoteOff: handleMidiNoteOff,
    onSustain: handleMidiSustain,
  });

  // Global keyboard shortcuts (space / arrows / K / L / M)
  useKeyboardShortcuts();

  // Calculate dynamic note range based on enabled tracks (only when fitKeyboardToSong is enabled)
  const { minNote, maxNote } = useMemo(
    () =>
      settings.fitKeyboardToSong
        ? calculateNoteRange(currentFile, 2)
        : calculateNoteRange(null, 2),
    // eslint-disable-next-line react-hooks/exhaustive-deps
    [currentFile, currentFile?.tracks, settings.fitKeyboardToSong],
  );

  // Apply theme to document
  useEffect(() => {
    document.documentElement.setAttribute('data-theme', settings.theme);
  }, [settings.theme]);

  // Keep the document language (and window title) in sync with the UI language
  useEffect(() => {
    document.documentElement.setAttribute('lang', HTML_LANG[language]);
    document.title =
      language === 'zh' ? 'NoteRain 雨音 · 钢琴练习' : 'NoteRain · Piano practice';
    // Desktop shell: rebuild the native menu in the same language
    getDesktopApi()?.setLanguage(language);
  }, [language]);

  // Desktop shell: open MIDI files from the menu / CLI / "open with"
  useEffect(() => {
    const api = getDesktopApi();
    if (!api) return;
    return api.onOpenMidiFile((file) => {
      void loadFile(
        new File([file.data], file.name, { type: 'audio/midi' }),
      );
    });
  }, [loadFile]);

  // Handle key press on virtual piano
  const handleNoteOn = useCallback(
    async (noteNumber: number) => {
      if (isLoading) return; // Block input while loading
      await resumeAudio();
      noteOn(noteNumber, 64);
      setLiveNote(noteNumber, true);
      addSatisfiedWaitNote(noteNumber);
    },
    [noteOn, resumeAudio, setLiveNote, addSatisfiedWaitNote, isLoading],
  );

  const handleNoteOff = useCallback(
    (noteNumber: number) => {
      if (isLoading) return; // Block input while loading
      noteOff(noteNumber);
      setLiveNote(noteNumber, false);
    },
    [noteOff, setLiveNote, isLoading],
  );

  // --- Drag & drop -------------------------------------------------------

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  }, []);

  const handleDragEnter = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    dragDepth.current += 1;
    if (e.dataTransfer?.types?.includes('Files')) {
      setIsDragging(true);
    }
  }, []);

  const handleDragLeave = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    dragDepth.current = Math.max(0, dragDepth.current - 1);
    if (dragDepth.current === 0) setIsDragging(false);
  }, []);

  const handleDropFiles = useCallback(
    async (e: React.DragEvent) => {
      dragDepth.current = 0;
      setIsDragging(false);
      await handleDrop(e);
    },
    [handleDrop],
  );

  const handleLoadSample = useCallback(async () => {
    setIsLoadingSample(true);
    try {
      await loadFromUrl(SAMPLE_SONG_URL, SAMPLE_SONG_NAME);
    } catch (error) {
      console.warn('[App] Failed to load the bundled sample song:', error);
    } finally {
      setIsLoadingSample(false);
    }
  }, [loadFromUrl]);

  return (
    <div
      className="app"
      onDrop={handleDropFiles}
      onDragOver={handleDragOver}
      onDragEnter={handleDragEnter}
      onDragLeave={handleDragLeave}
    >
      {isLoading && (
        <LoadingScreen
          phase={loadingPhase}
          downloadProgress={downloadProgress}
          decodeProgress={decodeProgress}
          totalSamples={totalSamples}
          totalLayers={totalLayers}
        />
      )}

      <Controls isLoadingFullVelocity={isLoadingFullVelocity} />

      <div className="main-content">
        {settings.showNotesOverlay && <NotesOverlay />}

        {currentFile && settings.showSheetMusic && (
          <SheetMusic key={currentFile.id} />
        )}

        {currentFile && settings.showFallingNotes && !settings.showSheetMusic && (
          <FallingNotes
            key={currentFile.id}
            lookahead={3}
            minNote={minNote}
            maxNote={maxNote}
          />
        )}

        {!currentFile && !isLoading && (
          <div className="empty-state">
            <div className="drop-card">
              <div className="drop-icon">
                <FileMusic size={42} strokeWidth={1.5} />
              </div>
              <h1 className="drop-title">{t('app.emptyTitle')}</h1>
              <p className="drop-hint">{t('app.emptyHint')}</p>
              <div className="empty-actions">
                <button
                  className="empty-button cta"
                  onClick={() => emptyFileInputRef.current?.click()}
                >
                  <FolderOpen size={16} />
                  <span>{t('controls.openMidi')}</span>
                </button>
                <button
                  className="empty-button ghost"
                  onClick={handleLoadSample}
                  disabled={isLoadingSample}
                >
                  <Music size={16} />
                  <span>
                    {isLoadingSample
                      ? t('app.loadingFile')
                      : t('app.emptySample')}
                  </span>
                </button>
              </div>
              <input
                ref={emptyFileInputRef}
                type="file"
                accept=".mid,.midi"
                onChange={handleFileInput}
                style={{ display: 'none' }}
              />
              <span className="drop-meta">{t('app.emptyFormats')}</span>
              <span className="drop-shortcuts">{t('app.shortcuts')}</span>
            </div>
          </div>
        )}
      </div>

      <PianoKeyboard
        onNoteOn={handleNoteOn}
        onNoteOff={handleNoteOff}
        minNote={minNote}
        maxNote={maxNote}
      />

      {isDragging && (
        <div className="drag-overlay">
          <div className="drag-overlay-inner">{t('app.emptyDrop')}</div>
        </div>
      )}
    </div>
  );
}

export default App;
