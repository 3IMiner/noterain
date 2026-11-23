import { useEffect, useMemo, useSyncExternalStore } from 'react';
import { useMidiStore } from '../stores/midiStore';
import {
  pianoEngine,
  TOTAL_INITIAL_LAYERS,
  TOTAL_INITIAL_SAMPLES,
} from '../lib/audio/pianoEngine';

export type { LoadingPhase } from '../lib/audio/pianoEngine';

/**
 * Audio engine hook.
 *
 * All state lives in the shared `pianoEngine` singleton — this hook only
 * subscribes to it and forwards the user's audio settings. Calling it from
 * several components (toolbar, transport, live MIDI input) is free.
 */
export function useAudioEngine() {
  const status = useSyncExternalStore(
    pianoEngine.subscribe,
    pianoEngine.getStatus,
    pianoEngine.getStatus,
  );

  const volume = useMidiStore((s) => s.settings.volume);
  const audioEnabled = useMidiStore((s) => s.settings.audioEnabled);
  const playMidiInputAudio = useMidiStore((s) => s.settings.playMidiInputAudio);

  // Kick off the initial layer load once per app lifetime.
  useEffect(() => {
    void pianoEngine.start();
  }, []);

  // Keep the engine in sync with the user's audio settings.
  useEffect(() => {
    pianoEngine.setVolume(volume, audioEnabled);
  }, [volume, audioEnabled]);

  // Load the remaining velocity layers only while live MIDI input is on.
  useEffect(() => {
    pianoEngine.setFullVelocityEnabled(playMidiInputAudio);
  }, [playMidiInputAudio]);

  // Stable action identities so dependents don't re-subscribe every render.
  const actions = useMemo(
    () => ({
      playNote: (noteNumber: number, velocity?: number, duration?: number) =>
        pianoEngine.playNote(noteNumber, velocity, duration),
      noteOn: (noteNumber: number, velocity?: number) =>
        pianoEngine.noteOn(noteNumber, velocity),
      noteOff: (noteNumber: number) => pianoEngine.noteOff(noteNumber),
      stopAll: () => pianoEngine.stopAll(),
      setSustain: (isPressed: boolean) => pianoEngine.setSustain(isPressed),
      resumeAudio: () => pianoEngine.resume(),
    }),
    [],
  );

  return {
    ...actions,
    isLoaded: status.isLoaded,
    isLoading: !status.isLoaded,
    loadingPhase: status.phase,
    downloadProgress: status.downloadProgress,
    decodeProgress: status.decodeProgress,
    totalSamples: TOTAL_INITIAL_SAMPLES,
    totalLayers: TOTAL_INITIAL_LAYERS,
    isLoadingFullVelocity: status.isLoadingFullVelocity,
    fullVelocityProgress: status.fullVelocityProgress,
  };
}
