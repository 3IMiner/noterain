'use strict';

/**
 * Preload bridge: the only surface the renderer's page sees from the desktop
 * shell. Everything the page needs is exposed here explicitly — no node
 * integration is enabled in the renderer.
 */

const { contextBridge, ipcRenderer } = require('electron');

function readVersion() {
  const prefix = '--noterain-version=';
  const arg = process.argv.find((value) => value.startsWith(prefix));
  return arg ? arg.slice(prefix.length) : '';
}

contextBridge.exposeInMainWorld('noterainDesktop', {
  isDesktop: true,
  version: readVersion(),
  platform: process.platform,
  /** Native "open file" dialog; resolves to { name, data } or null. */
  openMidiDialog: () => ipcRenderer.invoke('noterain:pick-midi'),
  /** Tell the shell which language the UI is in (so menus follow it). */
  setLanguage: (language) => {
    ipcRenderer.send('noterain:set-language', language);
  },
  /**
   * Files opened from the menu, the command line, or "open with".
   * Returns an unsubscribe function.
   */
  onOpenMidiFile: (callback) => {
    const listener = (_event, payload) => {
      try {
        callback(payload);
      } catch (error) {
        console.error('[preload] open-midi handler failed:', error);
      }
    };
    ipcRenderer.on('noterain:open-midi', listener);
    return () => ipcRenderer.removeListener('noterain:open-midi', listener);
  },
});
