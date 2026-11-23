// NoteRain Native — Direct2D canvas + immediate-mode widgets
#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <windows.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "ui/theme.h"

namespace nr {

struct Rect {
  float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;

  Rect() = default;
  Rect(float px, float py, float pw, float ph) : x(px), y(py), w(pw), h(ph) {}

  bool contains(float px, float py) const {
    return px >= x && py >= y && px <= x + w && py <= y + h;
  }
  float right() const { return x + w; }
  float bottom() const { return y + h; }
  float cx() const { return x + w * 0.5f; }
  float cy() const { return y + h * 0.5f; }
  Rect inset(float d) const { return Rect(x + d, y + d, w - 2 * d, h - 2 * d); }
  Rect offset(float dx, float dy) const { return Rect(x + dx, y + dy, w, h); }
};

enum class Align { Left, Center, Right };
enum class VAlign { Top, Middle, Bottom };

/**
 * Thin immediate-mode UI layer over Direct2D + DirectWrite.
 *
 * All coordinates are logical units at 96 dpi; the canvas applies the DPI scale
 * transform so layout code never worries about it.
 */
class Canvas {
public:
  bool init(HWND hwnd);
  void shutdown();
  void resize(UINT pixelWidth, UINT pixelHeight, float dpiScale);

  void beginFrame();
  void endFrame();

  float width() const { return width_; }
  float height() const { return height_; }
  float dpiScale() const { return scale_; }

  void onMouseMove(float pixelX, float pixelY);
  void onMouseDown();
  void onMouseUp();
  void onWheel(int delta);
  float mouseX() const { return mx_; }
  float mouseY() const { return my_; }
  bool mouseHeld() const { return mouseHeld_; }
  int wheelDelta() const { return wheelDelta_; }

  // Drawing primitives -----------------------------------------------------
  void fill(Rect r, Color c);
  void roundRect(Rect r, float radius, Color c);
  void strokeRect(Rect r, float radius, Color c, float width = 1.0f);
  void circle(float cx, float cy, float radius, Color c);
  void line(float x1, float y1, float x2, float y2, Color c, float width = 1.0f);
  void triangle(float x1, float y1, float x2, float y2, float x3, float y3, Color c);
  void text(const std::string& utf8, Rect r, Color c, float size,
            Align h = Align::Left, VAlign v = VAlign::Middle, bool bold = false,
            bool mono = false);
  /** Multi-line text: wraps inside the rect instead of clipping. */
  void textWrapped(const std::string& utf8, Rect r, Color c, float size,
                   Align h = Align::Left, VAlign v = VAlign::Top, bool bold = false);
  float textWidth(const std::string& utf8, float size, bool bold = false, bool mono = false);
  void pushClip(Rect r);
  void popClip();

  // Widgets ----------------------------------------------------------------
  bool button(int id, Rect r, const std::string& label, const Theme& t,
              bool primary = false, bool enabled = true, float fontSize = 13.0f);
  bool toggler(int id, Rect r, const std::string& label, bool& value, const Theme& t);
  int segmented(int id, Rect r, const std::vector<std::string>& items, int& value,
                const Theme& t);
  bool slider(int id, Rect r, float& value, float minValue, float maxValue, const Theme& t,
              const std::string& suffix = "");
  void dropdown(int id, Rect r, const std::vector<std::string>& items, int& value,
                const Theme& t, bool enabled = true);
  bool stepper(int id, Rect r, int& value, int minValue, int maxValue, const Theme& t);

  /** Consume a press inside `r` (for custom-drawn buttons). */
  bool hitButton(Rect r);
  bool isHovered(Rect r) const { return r.contains(mx_, my_); }

private:
  struct WidgetState {
    bool open = false;
    int committed = -1;
  };
  struct PopupRequest {
    int id = 0;
    Rect anchor;
    std::vector<std::string> items;
    int current = 0;
    const Theme* theme = nullptr;
  };

  IDWriteTextFormat* formatFor(float size, bool bold, bool mono, Align h, VAlign v,
                               bool wrap = false);
  void drawPopups();

  HWND hwnd_ = nullptr;
  ID2D1Factory* factory_ = nullptr;
  ID2D1HwndRenderTarget* target_ = nullptr;
  ID2D1SolidColorBrush* brush_ = nullptr;
  IDWriteFactory* writeFactory_ = nullptr;

  std::unordered_map<int, IDWriteTextFormat*> formats_;
  std::unordered_map<std::string, float> textWidths_;
  std::unordered_map<int, WidgetState> widgets_;
  std::vector<PopupRequest> popups_;

  float scale_ = 1.0f;
  float width_ = 1280.0f;
  float height_ = 800.0f;
  float mx_ = 0.0f;
  float my_ = 0.0f;
  bool mouseHeld_ = false;
  bool mousePressed_ = false;
  bool clickConsumed_ = false;
  int wheelDelta_ = 0;
  int openPopup_ = 0;
  int dragSlider_ = 0;
  bool popupJustOpened_ = false;
};

} // namespace nr
