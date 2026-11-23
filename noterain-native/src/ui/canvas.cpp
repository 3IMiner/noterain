#include "ui/canvas.h"

#include <algorithm>
#include <cmath>

namespace nr {
namespace {

std::wstring widen(const std::string& utf8) {
  if (utf8.empty()) return {};
  const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
  if (size <= 0) return {};
  std::wstring out((size_t)size, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), out.data(), size);
  return out;
}

D2D1_RECT_F toD2D(const Rect& r) { return D2D1_RECT_F{r.x, r.y, r.x + r.w, r.y + r.h}; }

D2D1_ROUNDED_RECT toRound(const Rect& r, float radius) {
  const float rad = std::min(radius, std::min(r.w, r.h) * 0.5f);
  return D2D1_ROUNDED_RECT{toD2D(r), rad, rad};
}

constexpr D2D1_MATRIX_3X2_F kIdentity{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};

} // namespace

bool Canvas::init(HWND hwnd) {
  hwnd_ = hwnd;

  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), nullptr,
                               (void**)&factory_)) ||
      !factory_) {
    return false;
  }

  if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                 (IUnknown**)&writeFactory_)) ||
      !writeFactory_) {
    return false;
  }
  return true;
}

void Canvas::shutdown() {
  for (auto& [key, format] : formats_) {
    (void)key;
    if (format) format->Release();
  }
  formats_.clear();
  textWidths_.clear();
  if (brush_) {
    brush_->Release();
    brush_ = nullptr;
  }
  if (target_) {
    target_->Release();
    target_ = nullptr;
  }
  if (writeFactory_) {
    writeFactory_->Release();
    writeFactory_ = nullptr;
  }
  if (factory_) {
    factory_->Release();
    factory_ = nullptr;
  }
}

void Canvas::resize(UINT pixelWidth, UINT pixelHeight, float dpiScale) {
  if (pixelWidth == 0 || pixelHeight == 0) return;
  scale_ = dpiScale > 0.1f ? dpiScale : 1.0f;
  width_ = (float)pixelWidth / scale_;
  height_ = (float)pixelHeight / scale_;

  if (!factory_) return;

  if (target_) {
    target_->Resize(D2D1_SIZE_U{pixelWidth, pixelHeight});
    return;
  }

  D2D1_RENDER_TARGET_PROPERTIES props{};
  props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
  props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
  props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
  props.dpiX = 96.0f;
  props.dpiY = 96.0f;
  props.usage = D2D1_RENDER_TARGET_USAGE_NONE;
  props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

  D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps{};
  hwndProps.hwnd = hwnd_;
  hwndProps.pixelSize = D2D1_SIZE_U{pixelWidth, pixelHeight};
  hwndProps.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

  if (FAILED(factory_->CreateHwndRenderTarget(props, hwndProps, &target_)) || !target_) {
    target_ = nullptr;
    return;
  }
  target_->CreateSolidColorBrush(D2D1_COLOR_F{1.0f, 1.0f, 1.0f, 1.0f}, &brush_);
}

void Canvas::beginFrame() {
  if (!target_ || !brush_) return;
  target_->BeginDraw();
  const D2D1_MATRIX_3X2_F scaleMatrix{scale_, 0.0f, 0.0f, scale_, 0.0f, 0.0f};
  target_->SetTransform(scaleMatrix);
  clickConsumed_ = openPopup_ != 0;
  popups_.clear();
}

void Canvas::endFrame() {
  if (!target_) return;
  if (openPopup_ != 0) drawPopups();
  target_->SetTransform(kIdentity);
  const HRESULT hr = target_->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET) {
    if (brush_) {
      brush_->Release();
      brush_ = nullptr;
    }
    target_->Release();
    target_ = nullptr;
  }
  mousePressed_ = false;
  wheelDelta_ = 0;
  popupJustOpened_ = false;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void Canvas::onMouseMove(float pixelX, float pixelY) {
  mx_ = pixelX / scale_;
  my_ = pixelY / scale_;
}
void Canvas::onMouseDown() { mousePressed_ = true; mouseHeld_ = true; }
void Canvas::onMouseUp() {
  mouseHeld_ = false;
  dragSlider_ = 0;
}
void Canvas::onWheel(int delta) { wheelDelta_ += delta; }

// ---------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------

void Canvas::fill(Rect r, Color c) {
  if (!brush_ || r.w <= 0.0f || r.h <= 0.0f) return;
  brush_->SetColor(c.d2d());
  target_->FillRectangle(toD2D(r), brush_);
}

void Canvas::roundRect(Rect r, float radius, Color c) {
  if (!brush_ || r.w <= 0.0f || r.h <= 0.0f) return;
  brush_->SetColor(c.d2d());
  target_->FillRoundedRectangle(toRound(r, radius), brush_);
}

void Canvas::strokeRect(Rect r, float radius, Color c, float width) {
  if (!brush_ || r.w <= 0.0f || r.h <= 0.0f) return;
  brush_->SetColor(c.d2d());
  target_->DrawRoundedRectangle(toRound(r, radius), brush_, width);
}

void Canvas::circle(float cx, float cy, float radius, Color c) {
  if (!brush_) return;
  brush_->SetColor(c.d2d());
  target_->FillEllipse(D2D1_ELLIPSE{D2D1_POINT_2F{cx, cy}, radius, radius}, brush_);
}

void Canvas::line(float x1, float y1, float x2, float y2, Color c, float width) {
  if (!brush_) return;
  brush_->SetColor(c.d2d());
  target_->DrawLine(D2D1_POINT_2F{x1, y1}, D2D1_POINT_2F{x2, y2}, brush_, width);
}

void Canvas::triangle(float x1, float y1, float x2, float y2, float x3, float y3, Color c) {
  if (!factory_ || !brush_ || !target_) return;
  ID2D1PathGeometry* geometry = nullptr;
  if (FAILED(factory_->CreatePathGeometry(&geometry)) || !geometry) return;
  ID2D1GeometrySink* sink = nullptr;
  if (FAILED(geometry->Open(&sink)) || !sink) {
    geometry->Release();
    return;
  }
  sink->BeginFigure(D2D1_POINT_2F{x1, y1}, D2D1_FIGURE_BEGIN_FILLED);
  sink->AddLine(D2D1_POINT_2F{x2, y2});
  sink->AddLine(D2D1_POINT_2F{x3, y3});
  sink->EndFigure(D2D1_FIGURE_END_CLOSED);
  sink->Close();
  sink->Release();
  brush_->SetColor(c.d2d());
  target_->FillGeometry(geometry, brush_);
  geometry->Release();
}

void Canvas::pushClip(Rect r) {
  if (!target_) return;
  target_->PushAxisAlignedClip(toD2D(r), D2D1_ANTIALIAS_MODE_ALIASED);
}

void Canvas::popClip() {
  if (!target_) return;
  target_->PopAxisAlignedClip();
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

IDWriteTextFormat* Canvas::formatFor(float size, bool bold, bool mono, Align h, VAlign v,
                                     bool wrap) {
  if (!writeFactory_) return nullptr;
  const int key = (int)std::lround(size * 10.0f) | ((bold ? 1 : 0) << 20) | ((mono ? 1 : 0) << 21) |
                  ((int)h << 22) | ((int)v << 24) | ((wrap ? 1 : 0) << 26);
  auto it = formats_.find(key);
  if (it != formats_.end()) return it->second;

  const wchar_t* families[4] = {L"Microsoft YaHei UI", L"Microsoft YaHei", L"Segoe UI", L"Arial"};
  if (mono) {
    families[0] = L"Consolas";
    families[1] = L"Cascadia Mono";
    families[2] = L"Courier New";
    families[3] = L"Segoe UI";
  }

  IDWriteTextFormat* format = nullptr;
  for (const wchar_t* family : families) {
    if (SUCCEEDED(writeFactory_->CreateTextFormat(
            family, nullptr, bold ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"zh-CN", &format)) &&
        format) {
      break;
    }
    format = nullptr;
  }
  if (!format) return nullptr;

  switch (h) {
    case Align::Left: format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING); break;
    case Align::Center: format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER); break;
    case Align::Right: format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); break;
  }
  switch (v) {
    case VAlign::Top: format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR); break;
    case VAlign::Middle: format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER); break;
    case VAlign::Bottom: format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR); break;
  }
  format->SetWordWrapping(wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
  formats_[key] = format;
  return format;
}

void Canvas::textWrapped(const std::string& utf8, Rect r, Color c, float size, Align h,
                         VAlign v, bool bold) {
  if (!target_ || !brush_ || utf8.empty()) return;
  IDWriteTextFormat* format = formatFor(size, bold, false, h, v, true);
  if (!format) return;
  const std::wstring wide = widen(utf8);
  if (wide.empty()) return;
  brush_->SetColor(c.d2d());
  target_->DrawTextW(wide.c_str(), (UINT32)wide.size(), format, toD2D(r), brush_,
                     D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
}

void Canvas::text(const std::string& utf8, Rect r, Color c, float size, Align h, VAlign v,
                  bool bold, bool mono) {
  if (!target_ || !brush_ || utf8.empty()) return;
  IDWriteTextFormat* format = formatFor(size, bold, mono, h, v);
  if (!format) return;
  const std::wstring wide = widen(utf8);
  if (wide.empty()) return;
  brush_->SetColor(c.d2d());
  target_->DrawTextW(wide.c_str(), (UINT32)wide.size(), format, toD2D(r), brush_,
                     D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
}

float Canvas::textWidth(const std::string& utf8, float size, bool bold, bool mono) {
  if (!writeFactory_ || utf8.empty()) return 0.0f;
  const std::string key = utf8 + "|" + std::to_string((int)(size * 10)) + (bold ? "b" : "") +
                          (mono ? "m" : "");
  auto it = textWidths_.find(key);
  if (it != textWidths_.end()) return it->second;

  IDWriteTextFormat* format = formatFor(size, bold, mono, Align::Left, VAlign::Top);
  if (!format) return 0.0f;
  const std::wstring wide = widen(utf8);
  IDWriteTextLayout* layout = nullptr;
  float width = (float)wide.size() * size * 0.55f;
  if (SUCCEEDED(writeFactory_->CreateTextLayout(wide.c_str(), (UINT32)wide.size(), format, 4096.0f,
                                                1000.0f, &layout)) &&
      layout) {
    DWRITE_TEXT_METRICS metrics{};
    if (SUCCEEDED(layout->GetMetrics(&metrics))) width = metrics.widthIncludingTrailingWhitespace;
    layout->Release();
  }
  textWidths_[key] = width;
  return width;
}

// ---------------------------------------------------------------------------
// Widgets
// ---------------------------------------------------------------------------

bool Canvas::button(int id, Rect r, const std::string& label, const Theme& t, bool primary,
                    bool enabled, float fontSize) {
  (void)id;
  const bool hovered = enabled && r.contains(mx_, my_);
  bool clicked = false;
  if (hovered && mousePressed_ && !clickConsumed_) {
    clicked = true;
    clickConsumed_ = true;
  }

  Color bg = primary ? t.accent : t.elevated;
  if (primary) {
    if (hovered) bg = bg.mix(Color(1.0f, 1.0f, 1.0f), 0.10f);
  } else if (hovered) {
    bg = t.hover;
  }
  if (!enabled) bg = bg.withAlpha(0.4f);

  roundRect(r, 8.0f, bg);
  if (!primary) strokeRect(r, 8.0f, hovered && enabled ? t.accent : t.border, 1.0f);

  Color fg = primary ? t.accentText : t.text;
  if (!enabled) fg = t.textMuted;
  text(label, r, fg, fontSize, Align::Center, VAlign::Middle, primary);
  return clicked;
}

bool Canvas::toggler(int id, Rect r, const std::string& label, bool& value, const Theme& t) {
  (void)id;
  bool changed = false;
  const bool hovered = r.contains(mx_, my_);
  if (hovered && mousePressed_ && !clickConsumed_) {
    value = !value;
    changed = true;
    clickConsumed_ = true;
  }

  const float pillW = 36.0f;
  const float pillH = 20.0f;
  const Rect pill(r.x, r.y + (r.h - pillH) * 0.5f, pillW, pillH);
  roundRect(pill, pillH * 0.5f, value ? t.accent : t.hover);
  circle(value ? pill.right() - 10.0f : pill.x + 10.0f, pill.cy(), 8.0f,
         value ? t.accentText : t.textDim);
  text(label, Rect(pill.right() + 10.0f, r.y, std::max(0.0f, r.w - pillW - 12.0f), r.h),
       hovered ? t.text : t.textDim, 13.0f);
  return changed;
}

int Canvas::segmented(int id, Rect r, const std::vector<std::string>& items, int& value,
                      const Theme& t) {
  if (items.empty()) return value;
  roundRect(r, 9.0f, t.elevated);
  const float inner = r.w - 8.0f;
  const float segW = inner / (float)items.size();

  for (int i = 0; i < (int)items.size(); ++i) {
    const Rect cell(r.x + 4.0f + segW * (float)i, r.y + 4.0f, segW, r.h - 8.0f);
    const bool selected = i == value;
    const bool hovered = cell.contains(mx_, my_);
    if (selected) roundRect(cell, 7.0f, t.accent);
    else if (hovered) roundRect(cell, 7.0f, t.hover.withAlpha(0.6f));
    text(items[(size_t)i], cell, selected ? t.accentText : (hovered ? t.text : t.textDim), 12.5f,
         Align::Center, VAlign::Middle, selected);
    if (hovered && mousePressed_ && !clickConsumed_) {
      value = i;
      clickConsumed_ = true;
    }
  }
  (void)id;
  return value;
}

bool Canvas::slider(int id, Rect r, float& value, float minValue, float maxValue, const Theme& t,
                    const std::string& suffix) {
  bool changed = false;
  const float x0 = r.x + 9.0f;
  const float x1 = r.right() - (suffix.empty() ? 9.0f : 54.0f);
  const float span = std::max(1.0f, x1 - x0);

  if (mousePressed_ && !clickConsumed_ && r.contains(mx_, my_)) {
    dragSlider_ = id;
    clickConsumed_ = true;
  }
  if (dragSlider_ == id && mouseHeld_) {
    float ratio = std::clamp((mx_ - x0) / span, 0.0f, 1.0f);
    const float next = minValue + (maxValue - minValue) * ratio;
    if (std::fabs(next - value) > 1e-4f) {
      value = next;
      changed = true;
    }
  }

  const float ratio = std::clamp((value - minValue) / std::max(1e-6f, maxValue - minValue), 0.0f, 1.0f);
  roundRect(Rect(x0, r.cy() - 3.0f, span, 6.0f), 3.0f, t.hover);
  roundRect(Rect(x0, r.cy() - 3.0f, span * ratio, 6.0f), 3.0f, t.accent);
  circle(x0 + span * ratio, r.cy(), 7.0f, t.text);

  if (!suffix.empty()) {
    text(suffix, Rect(x1 + 10.0f, r.y, 44.0f, r.h), t.textDim, 11.5f, Align::Right,
         VAlign::Middle, false, true);
  }
  return changed;
}

void Canvas::dropdown(int id, Rect r, const std::vector<std::string>& items, int& value,
                      const Theme& t, bool enabled) {
  WidgetState& state = widgets_[id];
  if (state.committed >= 0) {
    value = state.committed;
    state.committed = -1;
  }

  const bool hovered = enabled && r.contains(mx_, my_);
  roundRect(r, 8.0f, enabled ? (hovered ? t.hover : t.elevated) : t.elevated.withAlpha(0.4f));
  strokeRect(r, 8.0f, openPopup_ == id ? t.accent : (hovered ? t.accent : t.border), 1.0f);

  const std::string label =
      (value >= 0 && value < (int)items.size()) ? items[(size_t)value] : std::string("-");
  text(label, Rect(r.x + 10.0f, r.y, r.w - 32.0f, r.h), enabled ? t.text : t.textMuted, 12.5f);

  const float ax = r.right() - 16.0f;
  const float ay = r.cy() - 2.0f;
  triangle(ax - 4.0f, ay, ax + 4.0f, ay, ax, ay + 5.0f, enabled ? t.textDim : t.textMuted);

  // Clicking the control toggles the popup; a click while it is open closes it,
  // so this branch deliberately ignores the frame-wide click guard.
  const bool canClick = mousePressed_ && (!clickConsumed_ || openPopup_ == id);
  if (enabled && hovered && canClick) {
    clickConsumed_ = true;
    if (openPopup_ == id) {
      openPopup_ = 0;
    } else {
      openPopup_ = id;
      popupJustOpened_ = true;
    }
  }

  if (openPopup_ == id) {
    PopupRequest request;
    request.id = id;
    request.anchor = r;
    request.items = items;
    request.current = value;
    request.theme = &t;
    popups_.push_back(std::move(request));
  }
}

bool Canvas::stepper(int id, Rect r, int& value, int minValue, int maxValue, const Theme& t) {
  bool changed = false;
  const float size = std::min(r.h, 26.0f);
  const Rect minus(r.x, r.cy() - size * 0.5f, size, size);
  const Rect plus(r.right() - size, r.cy() - size * 0.5f, size, size);
  const Rect middle(minus.right() + 4.0f, r.y, std::max(0.0f, plus.x - minus.right() - 8.0f), r.h);

  auto drawButton = [&](const Rect& box, const char* glyph, bool up) {
    const bool hovered = box.contains(mx_, my_);
    roundRect(box, 6.0f, hovered ? t.hover : t.elevated);
    strokeRect(box, 6.0f, t.border, 1.0f);
    text(glyph, box, t.text, 14.0f, Align::Center, VAlign::Middle, false, false);
    if (hovered && mousePressed_ && !clickConsumed_) {
      clickConsumed_ = true;
      const int next = value + (up ? 1 : -1);
      if (next >= minValue && next <= maxValue && next != value) {
        value = next;
        changed = true;
      }
    }
  };

  drawButton(minus, "\xE2\x88\x92", false); // U+2212 minus
  drawButton(plus, "+", true);
  text(std::to_string(value), middle, t.text, 13.0f, Align::Center, VAlign::Middle, false, true);
  (void)id;
  return changed;
}

bool Canvas::hitButton(Rect r) {
  if (mousePressed_ && !clickConsumed_ && r.contains(mx_, my_)) {
    clickConsumed_ = true;
    return true;
  }
  return false;
}

void Canvas::drawPopups() {
  if (popups_.empty()) return;
  PopupRequest* request = nullptr;
  for (auto& p : popups_) {
    if (p.id == openPopup_) request = &p;
  }
  if (!request || !request->theme) return;
  const Theme& t = *request->theme;

  const int count = (int)request->items.size();
  const float itemH = 30.0f;
  const float boxH = std::min((float)count * itemH + 8.0f, 340.0f);
  float bx = request->anchor.x;
  float by = request->anchor.bottom() + 4.0f;
  const float bw = std::max(request->anchor.w, 190.0f);
  if (by + boxH > height_ - 8.0f) by = std::max(8.0f, request->anchor.y - boxH - 4.0f);
  if (bx + bw > width_ - 8.0f) bx = std::max(8.0f, width_ - bw - 8.0f);
  const Rect box(bx, by, bw, boxH);

  roundRect(box.offset(0.0f, 3.0f), 9.0f, Color(0.0f, 0.0f, 0.0f, 0.22f));
  roundRect(box, 9.0f, t.panel);
  strokeRect(box, 9.0f, t.border, 1.0f);

  pushClip(box);
  for (int i = 0; i < count; ++i) {
    const Rect item(box.x + 4.0f, box.y + 4.0f + itemH * (float)i, box.w - 8.0f, itemH);
    if (item.y + item.h > box.bottom()) break;
    const bool selected = i == request->current;
    const bool hovered = item.contains(mx_, my_);
    if (hovered) roundRect(item, 6.0f, t.hover);
    text(request->items[(size_t)i], Rect(item.x + 10.0f, item.y, item.w - 20.0f, item.h),
         selected ? t.accent : t.text, 12.5f, Align::Left, VAlign::Middle, selected);
    if (hovered && mousePressed_ && !popupJustOpened_) {
      widgets_[request->id].committed = i;
      openPopup_ = 0;
      clickConsumed_ = true;
    }
  }
  popClip();

  if (mousePressed_ && !popupJustOpened_ && !clickConsumed_ && !box.contains(mx_, my_) &&
      !request->anchor.contains(mx_, my_)) {
    openPopup_ = 0;
  }
}

} // namespace nr
