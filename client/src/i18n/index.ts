import { useCallback, useMemo } from 'react';
import { useMidiStore } from '../stores/midiStore';
import {
  chordQualityName,
  detectLanguage,
  LANGUAGE_CHIPS,
  LANGUAGE_LABELS,
  translate,
  type Language,
  type MessageKey,
} from './messages';

export type {
  Language,
  MessageKey,
} from './messages';
export {
  LANGUAGES,
  LANGUAGE_CHIPS,
  LANGUAGE_LABELS,
  HTML_LANG,
  detectLanguage,
  translate,
} from './messages';

export type Translate = (
  key: MessageKey,
  params?: Record<string, string | number>,
) => string;

/**
 * Language + translation hook.
 *
 * The language lives in the persisted settings store, so every component that
 * calls `useI18n()` re-renders the moment the user flips the switch — no
 * provider tree required.
 */
export function useI18n() {
  const stored = useMidiStore((s) => s.settings.language) as
    | Language
    | undefined;
  const language: Language = useMemo(
    () => stored ?? detectLanguage(),
    [stored],
  );

  const t = useCallback<Translate>(
    (key, params) => translate(language, key, params),
    [language],
  );

  const setLanguage = useCallback((next: Language) => {
    useMidiStore.getState().updateSettings({ language: next });
  }, []);

  const toggleLanguage = useCallback(() => {
    setLanguage(language === 'zh' ? 'en' : 'zh');
  }, [language, setLanguage]);

  const chordName = useCallback(
    (suffix: string) => chordQualityName(language, suffix),
    [language],
  );

  return {
    language,
    t,
    setLanguage,
    toggleLanguage,
    chordName,
    chip: LANGUAGE_CHIPS[language],
    label: LANGUAGE_LABELS[language],
  };
}
