#include "ui/theme.h"

namespace nr {

const Theme& darkTheme() {
  static Theme t = [] {
    Theme theme;
    theme.dark = true;
    theme.bg = Color::hex(0x1B1B25);
    theme.panel = Color::hex(0x16161F);
    theme.elevated = Color::hex(0x27273A);
    theme.hover = Color::hex(0x32324A);
    theme.border = Color::hex(0x363650);
    theme.text = Color::hex(0xDFE3F2);
    theme.textDim = Color::hex(0xA2A9C4);
    theme.textMuted = Color::hex(0x6E7694);
    theme.accent = Color::hex(0x74A8F7);
    theme.accentText = Color::hex(0x0E1020);
    theme.good = Color::hex(0x7ED9A0);
    theme.warn = Color::hex(0xF2CF77);
    theme.bad = Color::hex(0xF0708E);
    theme.hitLine = Color::hex(0x74A8F7, 0.85f);
    theme.keyWhite = Color::hex(0xF2F3F8);
    theme.keyWhiteTop = Color::hex(0xD8DBE8);
    theme.keyBlack = Color::hex(0x24243A);
    theme.keyActiveLeft = Color::hex(0x74A8F7);
    theme.keyActiveRight = Color::hex(0x7ED9A0);
    theme.keyHint = Color::hex(0xF2CF77);
    theme.track[0] = Color::hex(0x74A8F7);
    theme.track[1] = Color::hex(0x7ED9A0);
    theme.track[2] = Color::hex(0xE39BF0);
    theme.track[3] = Color::hex(0xF2CF77);
    theme.track[4] = Color::hex(0xF0938E);
    theme.track[5] = Color::hex(0x7FD8DE);
    theme.track[6] = Color::hex(0xB8A0F5);
    theme.track[7] = Color::hex(0xE7B27A);
    return theme;
  }();
  return t;
}

const Theme& lightTheme() {
  static Theme t = [] {
    Theme theme;
    theme.dark = false;
    theme.bg = Color::hex(0xF3F4F9);
    theme.panel = Color::hex(0xE9EBF3);
    theme.elevated = Color::hex(0xDDE0EB);
    theme.hover = Color::hex(0xD2D6E4);
    theme.border = Color::hex(0xC3C8D8);
    theme.text = Color::hex(0x252A3A);
    theme.textDim = Color::hex(0x4D5470);
    theme.textMuted = Color::hex(0x7D849D);
    theme.accent = Color::hex(0x2F6FD0);
    theme.accentText = Color::hex(0xFFFFFF);
    theme.good = Color::hex(0x1E8B52);
    theme.warn = Color::hex(0xA9761B);
    theme.bad = Color::hex(0xC0304C);
    theme.hitLine = Color::hex(0x2F6FD0, 0.85f);
    theme.keyWhite = Color::hex(0xFFFFFF);
    theme.keyWhiteTop = Color::hex(0xE8EAF2);
    theme.keyBlack = Color::hex(0x2A2F40);
    theme.keyActiveLeft = Color::hex(0x2F6FD0);
    theme.keyActiveRight = Color::hex(0x1E8B52);
    theme.keyHint = Color::hex(0xC08A16);
    theme.track[0] = Color::hex(0x2F6FD0);
    theme.track[1] = Color::hex(0x1E8B52);
    theme.track[2] = Color::hex(0x9A3FBF);
    theme.track[3] = Color::hex(0xB48213);
    theme.track[4] = Color::hex(0xC0473C);
    theme.track[5] = Color::hex(0x1B8891);
    theme.track[6] = Color::hex(0x6B4EDB);
    theme.track[7] = Color::hex(0xA35C1F);
    return theme;
  }();
  return t;
}

const Theme& themeAt(int index) { return index == 1 ? lightTheme() : darkTheme(); }

} // namespace nr
