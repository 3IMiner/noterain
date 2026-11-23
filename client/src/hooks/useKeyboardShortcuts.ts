import { useEffect } from 'react';
import { useMidiStore } from '../stores/midiStore';

/** True when the event comes from a control that handles keys itself. */
function isTypingTarget(target: EventTarget | null): boolean {
  const element = target as HTMLElement | null;
  if (!element) return false;
  const tag = element.tagName;
  return (
    tag === 'INPUT' ||
    tag === 'TEXTAREA' ||
    tag === 'SELECT' ||
    element.isContentEditable
  );
}

/**
 * Global keyboard shortcuts.
 *
 *   Space      play / pause
 *   ← / →      seek -5s / +5s
 *   ↑ / ↓      speed +5% / -5%
 *   K          stop
 *   L          toggle loop
 *   M          mute / unmute
 */
export function useKeyboardShortcuts() {
  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.defaultPrevented) return;
      if (event.metaKey || event.ctrlKey || event.altKey) return;
      if (isTypingTarget(event.target)) return;

      const state = useMidiStore.getState();
      const file = state.files.find((f) => f.id === state.currentFileId);
      const duration = file?.duration ?? 0;

      switch (event.code) {
        case 'Space': {
          event.preventDefault();
          if (!file) return;
          if (state.playback.isPlaying) {
            state.pause();
          } else {
            state.play();
          }
          return;
        }
        case 'ArrowLeft': {
          if (!file) return;
          event.preventDefault();
          state.seek(Math.max(0, state.playback.currentTime - 5));
          state.resetWaitModeState(
            Math.max(0, state.playback.currentTime - 5),
          );
          return;
        }
        case 'ArrowRight': {
          if (!file) return;
          event.preventDefault();
          const next = Math.min(duration, state.playback.currentTime + 5);
          state.seek(next);
          state.resetWaitModeState(next);
          return;
        }
        case 'ArrowUp': {
          event.preventDefault();
          state.setSpeed(state.playback.speed + 0.05);
          return;
        }
        case 'ArrowDown': {
          event.preventDefault();
          state.setSpeed(state.playback.speed - 0.05);
          return;
        }
        case 'KeyK': {
          event.preventDefault();
          state.stop();
          state.resetWaitModeState(0);
          return;
        }
        case 'KeyL': {
          if (!file) return;
          event.preventDefault();
          state.toggleLoop();
          return;
        }
        case 'KeyM': {
          event.preventDefault();
          state.updateSettings({ audioEnabled: !state.settings.audioEnabled });
          return;
        }
        default:
          return;
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);
}
