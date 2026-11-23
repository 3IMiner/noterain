// NoteRain Native — colour palette
#pragma once

#include <d2d1.h>

namespace nr {

struct Color {
  float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

  Color() = default;
  Color(float red, float green, float blue, float alpha = 1.0f)
      : r(red), g(green), b(blue), a(alpha) {}

  static Color hex(unsigned rgb, float alpha = 1.0f) {
    return Color(((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f,
                 (rgb & 0xFF) / 255.0f, alpha);
  }

  Color withAlpha(float alpha) const { return Color(r, g, b, alpha); }
  Color mix(const Color& other, float t) const {
    return Color(r + (other.r - r) * t, g + (other.g - g) * t, b + (other.b - b) * t,
                 a + (other.a - a) * t);
  }
  D2D1_COLOR_F d2d() const { return D2D1_COLOR_F{r, g, b, a}; }
};

struct Theme {
  bool dark = true;
  Color bg;
  Color panel;
  Color elevated;
  Color hover;
  Color border;
  Color text;
  Color textDim;
  Color textMuted;
  Color accent;
  Color accentText;
  Color good;
  Color warn;
  Color bad;
  Color hitLine;
  Color keyWhite;
  Color keyWhiteTop;
  Color keyBlack;
  Color keyActiveLeft;
  Color keyActiveRight;
  Color keyHint;
  Color track[8];
};

const Theme& darkTheme();
const Theme& lightTheme();
const Theme& themeAt(int index);

} // namespace nr
