import * as Tone from 'tone';

/**
 * Salamander Grand Piano V3 — multi-velocity sampler.
 *
 * A single engine instance is shared by the whole app. Previously every
 * `useAudioEngine()` call site mounted its own set of samplers, which meant the
 * samples were fetched, decoded and held in memory twice (once for the
 * transport, once for live MIDI input). The engine is intentionally a module
 * singleton: idempotent start-up, one sample cache, one volume, one loading
 * state — and React StrictMode's double effects are harmless.
 */

/** Sample notes available in Salamander (every minor third) */
const SAMPLE_NOTES = [
  'A0', 'C1', 'Ds1', 'Fs1', 'A1', 'C2', 'Ds2', 'Fs2', 'A2', 'C3',
  'Ds3', 'Fs3', 'A3', 'C4', 'Ds4', 'Fs4', 'A4', 'C5', 'Ds5', 'Fs5',
  'A5', 'C6', 'Ds6', 'Fs6', 'A6', 'C7', 'Ds7', 'Fs7', 'A7', 'C8',
] as const;

/** Velocity layers to load (1 = softest, 16 = loudest) */
const VELOCITY_LAYERS = [
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
] as const;

/** Default velocity layer loaded on start-up */
const DEFAULT_VELOCITY_LAYER = 8;

export type LoadingPhase = 'downloading' | 'decoding' | 'done';

export interface PianoEngineStatus {
  /** At least one velocity layer is decoded and playable */
  isLoaded: boolean;
  phase: LoadingPhase;
  /** Samples downloaded for the initial layer */
  downloadProgress: number;
  /** Samples decoded for the initial layer */
  decodeProgress: number;
  isLoadingFullVelocity: boolean;
  fullVelocityProgress: number;
}

function getBaseUrl(velocityLayer: number): string {
  return `/samples/piano/v${velocityLayer}/`;
}

/** Tone.js note name ("Ds4" → "D#4") */
function toToneName(sampleNote: string): string {
  return sampleNote.replace('Ds', 'D#').replace('Fs', 'F#');
}

/** Map MIDI velocity (0-127) to the closest velocity layer */
function getVelocityLayer(velocity: number): number {
  // Quadratic curve: requires stronger hits for higher layers
  const normalized = velocity / 127;
  const curved = normalized * normalized;
  const layer = Math.ceil(curved * 16);
  return Math.max(1, Math.min(16, layer));
}

/** Get the best available layer from loaded layers */
function getBestAvailableLayer(
  velocity: number,
  loadedLayers: Set<number>,
): number {
  const idealLayer = getVelocityLayer(velocity);
  if (loadedLayers.has(idealLayer)) return idealLayer;

  let nearest = DEFAULT_VELOCITY_LAYER;
  let minDistance = Math.abs(idealLayer - nearest);

  for (const layer of loadedLayers) {
    const distance = Math.abs(idealLayer - layer);
    if (distance < minDistance) {
      minDistance = distance;
      nearest = layer;
    }
  }

  return nearest;
}

/** MIDI note number → scientific pitch name ("C4") */
const NOTE_NAMES = [
  'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B',
] as const;

function getNoteName(noteNumber: number): string {
  const octave = Math.floor(noteNumber / 12) - 1;
  return `${NOTE_NAMES[noteNumber % 12]}${octave}`;
}

class PianoEngine {
  private samplers = new Map<number, Tone.Sampler>();
  private activeNoteSamplers = new Map<number, number>();
  private loadedLayers = new Set<number>();
  /** original sample URL → blob URL */
  private blobUrls = new Map<string, string>();

  private volumeDb = 0;
  private audioEnabled = true;

  private sustain = false;
  private sustainedNotes = new Set<number>();

  private startRequested = false;
  private initialPromise: Promise<void> | null = null;
  private wantsFullVelocity = false;
  private fullLoadAbort: AbortController | null = null;
  private fullLoadRunning = false;

  private listeners = new Set<() => void>();

  private status: PianoEngineStatus = {
    isLoaded: false,
    phase: 'downloading',
    downloadProgress: 0,
    decodeProgress: 0,
    isLoadingFullVelocity: false,
    fullVelocityProgress: 0,
  };

  // --- Store plumbing -----------------------------------------------------

  subscribe = (listener: () => void): (() => void) => {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  };

  getStatus = (): PianoEngineStatus => this.status;

  private patch(partial: Partial<PianoEngineStatus>) {
    this.status = { ...this.status, ...partial };
    for (const listener of this.listeners) listener();
  }

  // --- Lifecycle ----------------------------------------------------------

  /** Idempotent: starts the initial (single-layer) load exactly once. */
  start(): Promise<void> {
    if (!this.initialPromise) {
      this.startRequested = true;
      this.initialPromise = this.loadInitialLayer();
    }
    return this.initialPromise;
  }

  get hasStarted(): boolean {
    return this.startRequested;
  }

  private async loadInitialLayer() {
    const layer = DEFAULT_VELOCITY_LAYER;
    let downloaded = 0;

    console.log(
      `[AudioEngine] Downloading ${SAMPLE_NOTES.length} samples (layer ${layer})...`,
    );

    await Promise.all(
      SAMPLE_NOTES.map(async (note) => {
        const url = `${getBaseUrl(layer)}${note}v${layer}.mp3`;
        try {
          const response = await fetch(url);
          if (!response.ok) throw new Error(`HTTP ${response.status}`);
          const blob = await response.blob();
          this.blobUrls.set(url, URL.createObjectURL(blob));
        } catch (error) {
          console.warn(`[AudioEngine] Failed to download ${url}:`, error);
        }
        downloaded++;
        this.patch({ downloadProgress: downloaded });
      }),
    );

    console.log('[AudioEngine] Initial samples downloaded, decoding...');
    this.patch({ phase: 'decoding' });

    await new Promise<void>((resolve) => {
      const urls: Record<string, string> = {};
      for (const note of SAMPLE_NOTES) {
        const url = `${getBaseUrl(layer)}${note}v${layer}.mp3`;
        urls[toToneName(note)] = this.blobUrls.get(url) ?? url;
      }

      const sampler = new Tone.Sampler({
        urls,
        onload: () => {
          this.samplers.set(layer, sampler);
          this.loadedLayers.add(layer);
          sampler.volume.value = this.volumeDb;
          this.patch({
            isLoaded: true,
            phase: 'done',
            decodeProgress: 1,
          });
          console.log(`[AudioEngine] Default velocity layer ${layer} ready`);
          resolve();
        },
        onerror: (error) => {
          console.warn(
            `[AudioEngine] Failed to decode velocity layer ${layer}:`,
            error,
          );
          resolve();
        },
      }).toDestination();
      sampler.volume.value = this.volumeDb;
    });

    if (this.wantsFullVelocity) {
      void this.loadFullVelocity();
    }
  }

  /** Enable/disable the background loading of the remaining velocity layers. */
  setFullVelocityEnabled(enabled: boolean) {
    this.wantsFullVelocity = enabled;
    if (!enabled) {
      this.cancelFullVelocity();
      return;
    }
    if (this.status.isLoaded) {
      void this.loadFullVelocity();
    }
  }

  cancelFullVelocity() {
    this.fullLoadAbort?.abort();
    this.fullLoadAbort = null;
    this.fullLoadRunning = false;
    this.patch({ isLoadingFullVelocity: false });
  }

  private async loadFullVelocity() {
    if (this.fullLoadRunning) return;
    if (this.loadedLayers.size >= VELOCITY_LAYERS.length) return;

    const abortController = new AbortController();
    this.fullLoadAbort = abortController;
    this.fullLoadRunning = true;

    const remaining = VELOCITY_LAYERS.filter(
      (layer) => !this.loadedLayers.has(layer),
    );
    if (remaining.length === 0) {
      this.fullLoadRunning = false;
      return;
    }

    console.log(
      `[AudioEngine] Loading ${remaining.length} remaining velocity layers...`,
    );
    this.patch({ isLoadingFullVelocity: true, fullVelocityProgress: 0 });

    let done = 0;

    for (const layer of remaining) {
      if (abortController.signal.aborted) break;

      await Promise.all(
        SAMPLE_NOTES.map(async (note) => {
          const url = `${getBaseUrl(layer)}${note}v${layer}.mp3`;
          if (this.blobUrls.has(url)) return;
          try {
            const response = await fetch(url, {
              signal: abortController.signal,
            });
            if (!response.ok) throw new Error(`HTTP ${response.status}`);
            const blob = await response.blob();
            if (abortController.signal.aborted) return;
            this.blobUrls.set(url, URL.createObjectURL(blob));
          } catch (error) {
            if ((error as Error).name !== 'AbortError') {
              console.warn(`[AudioEngine] Failed to download ${url}:`, error);
            }
          }
        }),
      );

      if (abortController.signal.aborted) break;

      await new Promise<void>((resolve) => {
        const urls: Record<string, string> = {};
        for (const note of SAMPLE_NOTES) {
          const url = `${getBaseUrl(layer)}${note}v${layer}.mp3`;
          urls[toToneName(note)] = this.blobUrls.get(url) ?? url;
        }

        const sampler = new Tone.Sampler({
          urls,
          onload: () => {
            if (!abortController.signal.aborted) {
              this.samplers.set(layer, sampler);
              this.loadedLayers.add(layer);
              sampler.volume.value = this.volumeDb;
              done++;
              this.patch({ fullVelocityProgress: done / remaining.length });
              console.log(
                `[AudioEngine] Velocity layer ${layer} loaded (${done}/${remaining.length})`,
              );
            } else {
              sampler.dispose();
            }
            resolve();
          },
          onerror: (error) => {
            console.warn(
              `[AudioEngine] Failed to decode velocity layer ${layer}:`,
              error,
            );
            resolve();
          },
        }).toDestination();
        sampler.volume.value = this.volumeDb;
      });
    }

    this.fullLoadRunning = false;
    this.fullLoadAbort = null;
    if (!abortController.signal.aborted) {
      console.log('[AudioEngine] All velocity layers loaded');
      this.patch({ isLoadingFullVelocity: false });
    }
  }

  // --- Volume -------------------------------------------------------------

  setVolume(volume: number, enabled: boolean) {
    this.audioEnabled = enabled;
    this.volumeDb = enabled ? Tone.gainToDb(volume) : -Infinity;
    for (const sampler of this.samplers.values()) {
      sampler.volume.value = this.volumeDb;
    }
  }

  async resume(): Promise<void> {
    if (Tone.context.state !== 'running') {
      await Tone.start();
    }
  }

  // --- Playing ------------------------------------------------------------

  /** One-shot note with velocity-appropriate sample */
  playNote(noteNumber: number, velocity = 100, duration = 0.5) {
    if (!this.audioEnabled || !this.status.isLoaded) return;

    const layer = getBestAvailableLayer(velocity, this.loadedLayers);
    const sampler = this.samplers.get(layer);
    if (!sampler) return;

    try {
      sampler.triggerAttackRelease(
        getNoteName(noteNumber),
        duration,
        Tone.now(),
      );
    } catch (error) {
      console.warn('[AudioEngine] Failed to play note:', error);
    }
  }

  /** Start a sustained note */
  noteOn(noteNumber: number, velocity = 100) {
    if (!this.audioEnabled || !this.status.isLoaded) return;

    const layer = getBestAvailableLayer(velocity, this.loadedLayers);
    const sampler = this.samplers.get(layer);
    if (!sampler) return;

    try {
      sampler.triggerAttack(getNoteName(noteNumber), Tone.now());
      this.activeNoteSamplers.set(noteNumber, layer);
    } catch (error) {
      console.warn('[AudioEngine] Failed to trigger note on:', error);
    }
  }

  /** Release a sustained note (honours the sustain pedal) */
  noteOff(noteNumber: number) {
    if (!this.status.isLoaded) return;

    if (this.sustain) {
      this.sustainedNotes.add(noteNumber);
      return;
    }

    const layer = this.activeNoteSamplers.get(noteNumber);
    if (layer === undefined) return;

    const sampler = this.samplers.get(layer);
    if (!sampler) return;

    try {
      sampler.triggerRelease(getNoteName(noteNumber), Tone.now());
      this.activeNoteSamplers.delete(noteNumber);
    } catch (error) {
      console.warn('[AudioEngine] Failed to trigger note off:', error);
    }
  }

  stopAll() {
    for (const sampler of this.samplers.values()) {
      sampler.releaseAll();
    }
    this.activeNoteSamplers.clear();
    this.sustainedNotes.clear();
  }

  setSustain(pressed: boolean) {
    this.sustain = pressed;
    if (pressed) return;

    for (const noteNumber of this.sustainedNotes) {
      const layer = this.activeNoteSamplers.get(noteNumber);
      if (layer === undefined) continue;
      const sampler = this.samplers.get(layer);
      if (!sampler) continue;
      try {
        sampler.triggerRelease(getNoteName(noteNumber), Tone.now());
        this.activeNoteSamplers.delete(noteNumber);
      } catch (error) {
        console.warn('[AudioEngine] Failed to release sustained note:', error);
      }
    }
    this.sustainedNotes.clear();
  }
}

/** Shared piano engine */
export const pianoEngine = new PianoEngine();

export const TOTAL_INITIAL_SAMPLES = SAMPLE_NOTES.length;
export const TOTAL_INITIAL_LAYERS = 1;
