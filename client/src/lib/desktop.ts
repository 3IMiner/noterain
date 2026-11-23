/** Bridge exposed by the Electron preload script (`desktop/preload.js`). */

export interface DesktopMidiFile {
  name: string;
  data: ArrayBuffer;
}

export interface NoterainDesktopApi {
  readonly isDesktop: true;
  readonly version: string;
  readonly platform: string;
  /** Ask the main process for a MIDI file via the native open dialog. */
  openMidiDialog: () => Promise<DesktopMidiFile | null>;
  /** Keep the native menu language in sync with the in-app language. */
  setLanguage: (language: 'en' | 'zh') => void;
  /** Subscribe to MIDI files opened from the menu, CLI or file association. */
  onOpenMidiFile: (
    callback: (file: DesktopMidiFile) => void,
  ) => () => void;
}

declare global {
  interface Window {
    noterainDesktop?: NoterainDesktopApi;
  }
}

/** The desktop bridge, or `null` when running in a plain browser. */
export function getDesktopApi(): NoterainDesktopApi | null {
  if (typeof window === 'undefined') return null;
  return window.noterainDesktop ?? null;
}

export function isDesktop(): boolean {
  return getDesktopApi() !== null;
}
