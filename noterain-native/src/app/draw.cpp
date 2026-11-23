#include "app/app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace nr {
namespace {

bool isBlackKey(int key) {
  const int n = ((key % 12) + 12) % 12;
  return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
}

std::string noteName(int key) {
  static const char* kNames[12] = {"C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B"};
  if (key < 0 || key > 127) return {};
  const int pc = ((key % 12) + 12) % 12;
  const int octave = key / 12 - 1;
  return std::string(kNames[pc]) + std::to_string(octave);
}

std::string formatTime(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0.0) seconds = 0.0;
  const int total = (int)std::floor(seconds);
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%d:%02d", total / 60, total % 60);
  return buffer;
}

void drawAppMark(Canvas& c, Rect r, const Theme& t) {
  c.roundRect(r, r.w * 0.28f, t.accent);
  const float barW = r.w * 0.13f;
  const float base = r.bottom() - r.h * 0.20f;
  const float heights[3] = {0.40f, 0.60f, 0.28f};
  const float alphas[3] = {0.95f, 0.78f, 0.58f};
  for (int i = 0; i < 3; ++i) {
    const float bx = r.x + r.w * (0.25f + 0.25f * (float)i) - barW * 0.5f;
    const float bh = r.h * heights[i];
    c.roundRect(Rect(bx, base - bh, barW, bh), barW * 0.5f, Color(1.0f, 1.0f, 1.0f, alphas[i]));
  }
}

void drawPlayIcon(Canvas& c, Rect r, Color color, bool playing) {
  if (playing) {
    const float barW = r.w * 0.30f;
    const float gap = r.w * 0.14f;
    c.roundRect(Rect(r.cx() - barW - gap * 0.5f, r.y, barW, r.h), 1.5f, color);
    c.roundRect(Rect(r.cx() + gap * 0.5f, r.y, barW, r.h), 1.5f, color);
  } else {
    c.triangle(r.x, r.y - r.h * 0.06f, r.x, r.bottom() + r.h * 0.06f, r.right(), r.cy(), color);
  }
}

void drawStopIcon(Canvas& c, Rect r, Color color) { c.roundRect(r, 2.0f, color); }

void drawMetronomeIcon(Canvas& c, Rect r, Color color) {
  const float bottom = r.bottom() - 1.0f;
  const float top = r.y + 1.0f;
  c.line(r.cx() - r.w * 0.36f, bottom, r.cx() - r.w * 0.11f, top, color, 1.6f);
  c.line(r.cx() + r.w * 0.36f, bottom, r.cx() + r.w * 0.11f, top, color, 1.6f);
  c.line(r.cx() - r.w * 0.44f, bottom, r.cx() + r.w * 0.44f, bottom, color, 1.8f);
  c.line(r.cx(), bottom - r.h * 0.16f, r.cx() + r.w * 0.24f, r.y + r.h * 0.30f, color, 1.5f);
  c.circle(r.cx() + r.w * 0.24f, r.y + r.h * 0.30f, 1.8f, color);
}

void drawGearIcon(Canvas& c, Rect r, Color color, Color background) {
  const float rad = std::min(r.w, r.h) * 0.5f;
  c.circle(r.cx(), r.cy(), rad * 0.92f, color);
  for (int i = 0; i < 8; ++i) {
    const float angle = (float)i * 3.14159265f / 4.0f;
    const float x = r.cx() + std::cos(angle) * rad * 0.85f;
    const float y = r.cy() + std::sin(angle) * rad * 0.85f;
    c.circle(x, y, rad * 0.22f, color);
  }
  c.circle(r.cx(), r.cy(), rad * 0.52f, background);
}

} // namespace

// ---------------------------------------------------------------------------
// Root
// ---------------------------------------------------------------------------

void App::draw() {
  const Theme& t = theme();
  canvas_.fill(Rect(0.0f, 0.0f, canvas_.width(), canvas_.height()), t.bg);

  if (view_ == View::Settings) {
    drawSettings();
    return;
  }

  drawTopBar();
  if (song_.empty()) {
    drawEmptyState();
  } else {
    drawSongArea();
    drawPanel();
  }
  drawKeyboard();

  if (!errorBanner_.empty()) {
    canvas_.text(errorBanner_, Rect(topBarArea_.x + 18.0f, topBarArea_.bottom() + 6.0f,
                                    canvas_.width() - 36.0f, 20.0f),
                 t.bad, 11.5f);
  }
}

// ---------------------------------------------------------------------------
// Top bar
// ---------------------------------------------------------------------------

void App::drawTopBar() {
  const Theme& t = theme();
  const Rect bar = topBarArea_;
  canvas_.fill(bar, t.panel);
  canvas_.line(bar.x, bar.bottom() - 1.0f, bar.right(), bar.bottom() - 1.0f, t.border, 1.0f);

  drawAppMark(canvas_, Rect(18.0f, bar.cy() - 14.0f, 28.0f, 28.0f), t);
  canvas_.text(str_->appName, Rect(54.0f, 0.0f, 130.0f, bar.h), t.text, 14.0f, Align::Left,
               VAlign::Middle, true);

  const Rect openBtn(186.0f, 16.0f, 104.0f, 32.0f);
  if (canvas_.button(1, openBtn, str_->openMidi, t, false, true, 12.5f)) openFileDialog();

  const std::string title = song_.empty() ? std::string(str_->noSongLoaded) : song_.title;
  canvas_.text(title, Rect(openBtn.right() + 12.0f, 0.0f, 196.0f, bar.h),
               song_.empty() ? t.textMuted : t.textDim, 12.5f);

  // Transport ------------------------------------------------------------
  const float transportX = 508.0f;
  const Rect playBtn(transportX, 12.0f, 40.0f, 40.0f);
  const Rect stopBtn(transportX + 48.0f, 16.0f, 32.0f, 32.0f);

  const bool canPlay = !song_.empty();
  const bool playHover = canPlay && canvas_.isHovered(playBtn);
  canvas_.circle(playBtn.cx(), playBtn.cy(), playBtn.w * 0.5f,
                 canPlay ? (playHover ? t.accent.mix(Color(1, 1, 1), 0.12f) : t.accent)
                         : t.elevated);
  if (canPlay && canvas_.hitButton(playBtn)) togglePlay();
  drawPlayIcon(canvas_, playBtn.inset(13.0f), canPlay ? t.accentText : t.textMuted, playing_);

  const bool stopHover = canPlay && canvas_.isHovered(stopBtn);
  canvas_.circle(stopBtn.cx(), stopBtn.cy(), stopBtn.w * 0.5f,
                 stopHover ? t.hover : t.elevated);
  if (canPlay && canvas_.hitButton(stopBtn)) stopPlayback();
  drawStopIcon(canvas_, stopBtn.inset(12.0f), canPlay ? t.text : t.textMuted);

  const double duration = song_.empty() ? 0.0 : song_.duration;
  const std::string timeText = formatTime(time_) + " / " + formatTime(duration);
  const float timeX = stopBtn.right() + 14.0f;
  canvas_.text(timeText, Rect(timeX, 10.0f, 132.0f, 22.0f),
               song_.empty() ? t.textMuted : t.text, 12.5f, Align::Left, VAlign::Middle, false,
               true);

  if (!song_.empty()) {
    const std::string measureText =
        std::to_string(song_.measureAt(time_) + 1) + "/" + std::to_string(song_.measureCount()) +
        " " + str_->bar;
    canvas_.text(measureText, Rect(timeX, 32.0f, 132.0f, 16.0f), t.textMuted, 10.5f);
  }

  // Right cluster --------------------------------------------------------
  float rx = bar.right() - 16.0f;

  const Rect settingsBtn(rx - 36.0f, 14.0f, 36.0f, 36.0f);
  rx = settingsBtn.x - 8.0f;
  const Rect langBtn(rx - 56.0f, 16.0f, 56.0f, 32.0f);
  rx = langBtn.x - 8.0f;
  const Rect metroBtn(rx - 36.0f, 14.0f, 36.0f, 36.0f);
  rx = metroBtn.x - 8.0f;
  const Rect speedBox(rx - 76.0f, 0.0f, 76.0f, bar.h);

  // Speed read-out (also clickable to reset to 100%).
  const std::string speedText =
      std::to_string((int)std::lround(settings_.speed * 100.0f)) + "%";
  canvas_.text(speedText, Rect(speedBox.x, 0.0f, speedBox.w, bar.h), t.textDim, 12.0f,
               Align::Center, VAlign::Middle, false, true);
  if (canvas_.hitButton(speedBox) && !song_.empty()) {
    settings_.speed = 1.0f;
    markSettingsDirty();
  }

  // Metronome toggle
  const bool metroOn = settings_.metronome;
  const bool metroHover = canvas_.isHovered(metroBtn);
  canvas_.roundRect(metroBtn, 8.0f, metroOn ? t.accent : (metroHover ? t.hover : t.elevated));
  drawMetronomeIcon(canvas_, metroBtn.inset(9.0f), metroOn ? t.accentText : t.textDim);
  if (canvas_.hitButton(metroBtn)) {
    settings_.metronome = !metroOn;
    markSettingsDirty();
  }

  // Language chip (shows the language you would switch to)
  const bool langHover = canvas_.isHovered(langBtn);
  canvas_.roundRect(langBtn, 16.0f, langHover ? t.hover : t.elevated);
  canvas_.text(lang_ == Lang::Zh ? "EN" : "中文", langBtn, langHover ? t.text : t.textDim, 12.0f,
               Align::Center, VAlign::Middle, true);
  if (canvas_.hitButton(langBtn)) {
    settings_.language = lang_ == Lang::Zh ? 1 : 0;
    markSettingsDirty();
  }

  // Settings
  const bool gearHover = canvas_.isHovered(settingsBtn);
  if (gearHover) canvas_.roundRect(settingsBtn, 8.0f, t.hover);
  drawGearIcon(canvas_, settingsBtn.inset(10.0f), gearHover ? t.text : t.textDim, t.panel);
  if (canvas_.hitButton(settingsBtn)) view_ = View::Settings;

  // Progress bar --------------------------------------------------------
  if (!song_.empty()) {
    const float progressX = timeX + 144.0f;
    const float progressRight = speedBox.x - 12.0f;
    const Rect progress(progressX, bar.cy() - 9.0f,
                        std::max(90.0f, progressRight - progressX), 18.0f);
    drawProgressBar(progress);
  }
}

void App::drawProgressBar(Rect area) {
  const Theme& t = theme();
  const double duration = song_.empty() ? 0.0 : song_.duration;
  if (duration <= 0.0) return;

  // Loop region behind the bar.
  if (loopActive()) {
    const float startRatio = (float)(loopStartTime() / duration);
    const float endRatio = (float)(loopEndTime() / duration);
    const Rect band(area.x + area.w * startRatio, area.cy() - 5.0f,
                    std::max(2.0f, area.w * (endRatio - startRatio)), 10.0f);
    canvas_.roundRect(band, 3.0f, t.accent.withAlpha(0.18f));
  }

  float ratio = (float)(time_ / duration);
  canvas_.roundRect(Rect(area.x, area.cy() - 3.0f, area.w, 6.0f), 3.0f, t.hover);
  canvas_.roundRect(Rect(area.x, area.cy() - 3.0f, area.w * ratio, 6.0f), 3.0f, t.accent);
  canvas_.circle(area.x + area.w * ratio, area.cy(), 6.0f, t.text);

  if (canvas_.slider(7, area.inset(-3.0f), ratio, 0.0f, 1.0f, t)) {
    seekTo((double)ratio * duration);
  }
}

// ---------------------------------------------------------------------------
// Falling notes
// ---------------------------------------------------------------------------

void App::drawSongArea() {
  const Theme& t = theme();
  const Rect area = notesArea_;
  canvas_.fill(area, t.bg);

  if (song_.empty() || area.h < 40.0f) return;

  const double lookahead = std::max(1, settings_.lookahead);
  const float hitY = area.bottom() - 6.0f;
  const float usable = std::max(40.0f, area.h - 18.0f);
  const float pps = usable / (float)lookahead;
  const double t0 = time_ - 0.6;
  const double t1 = time_ + lookahead;

  // Key columns ---------------------------------------------------------
  const int lowKey = windowLow_;
  const int highKey = windowHigh();
  int whiteCount = 0;
  for (int key = lowKey; key <= highKey; ++key) {
    if (!isBlackKey(key)) ++whiteCount;
  }
  if (whiteCount <= 0) whiteCount = 52;
  const float whiteW = area.w / (float)whiteCount;
  const float blackW = whiteW * 0.62f;

  // Precompute one x/width per key (hot path: up to a few thousand notes).
  float keyX[128] = {};
  float keyW[128] = {};
  {
    int whiteIndex = 0;
    for (int key = lowKey; key <= highKey; ++key) {
      if (isBlackKey(key)) {
        keyX[key] = area.x + whiteW * (float)whiteIndex - blackW * 0.5f;
        keyW[key] = blackW;
      } else {
        keyX[key] = area.x + whiteW * (float)whiteIndex;
        keyW[key] = whiteW;
        ++whiteIndex;
      }
    }
  }

  auto columnFor = [&](int key) -> Rect {
    const int clamped = std::clamp(key, lowKey, highKey);
    return Rect(keyX[clamped], area.y, keyW[clamped], area.h);
  };

  // Column separators (very subtle).
  for (int key = lowKey; key <= highKey; ++key) {
    if (isBlackKey(key)) continue;
    const Rect column = columnFor(key);
    canvas_.line(column.x, area.y, column.x, area.bottom(),
                 t.border.withAlpha(t.dark ? 0.35f : 0.45f), 1.0f);
  }

  // Measure / beat grid ------------------------------------------------
  const int firstBar = std::max(0, song_.measureAt(t0));
  for (int bar = firstBar; bar < song_.measureCount(); ++bar) {
    const double measureStart = song_.measureStart(bar);
    if (measureStart > t1) break;
    const float y = hitY - (float)((measureStart - time_) * pps);
    if (y >= area.y && y <= area.bottom()) {
      canvas_.line(area.x, y, area.right(), y, t.border, 1.0f);
    }
    const int beats = std::max(1, song_.timeSigAt(measureStart).num * 4 /
                                      std::max(1, song_.timeSigAt(measureStart).den));
    const double barLength = song_.measureSeconds(measureStart);
    for (int beat = 1; beat < beats; ++beat) {
      const double beatTime = measureStart + barLength * (double)beat / (double)beats;
      if (beatTime < t0 || beatTime > t1) continue;
      const float by = hitY - (float)((beatTime - time_) * pps);
      if (by >= area.y && by <= area.bottom()) {
        canvas_.line(area.x, by, area.right(), by, t.border.withAlpha(0.35f), 1.0f);
      }
    }
  }

  // Hit line -----------------------------------------------------------
  canvas_.line(area.x, hitY, area.right(), hitY, t.hitLine, 2.0f);

  // Click to seek ------------------------------------------------------
  if (canvas_.hitButton(area)) {
    const double target = time_ + (double)((hitY - canvas_.mouseY()) / pps);
    seekTo(target);
  }

  // Notes --------------------------------------------------------------
  canvas_.pushClip(Rect(area.x, area.y, area.w, area.h - 4.0f));

  auto firstVisible = std::lower_bound(
      song_.ordered.begin(), song_.ordered.end(), t0,
      [](const Note* note, double value) { return note->start < value; });
  int index = (int)(firstVisible - song_.ordered.begin());
  index = std::max(0, index - 64);

  const double pulse = 0.5 + 0.5 * std::sin(wallClock_ * 6.0);
  bool anyAbove = false;
  bool anyBelow = false;
  int highestOut = highKey;
  int lowestOut = lowKey;

  for (; index < (int)song_.ordered.size(); ++index) {
    const Note* note = song_.ordered[(size_t)index];
    if (note->start > t1) break;
    if (note->end() < t0) continue;
    if (!song_.tracks[(size_t)note->track].enabled) continue;

    const bool outOfRange = note->key < lowKey || note->key > highKey;
    if (outOfRange) {
      if (note->key > highKey) {
        anyAbove = true;
        highestOut = std::max(highestOut, note->key);
      } else {
        anyBelow = true;
        lowestOut = std::min(lowestOut, note->key);
      }
    }

    const Rect column = columnFor(note->key);
    const float bottomY = hitY - (float)((note->start - time_) * pps);
    const float topY = bottomY - (float)(note->dur * pps);
    const float height = std::max(6.0f, bottomY - topY);
    if (bottomY < area.y - 40.0f) continue;
    if (topY > area.bottom() + 40.0f) continue;

    const bool target = practiceActive() && isTargetNote(note);
    const bool active = time_ >= note->start - 0.02 && time_ <= note->end();
    const bool waitingHere = waiting_ &&
                             std::find(pendingKeys_.begin(), pendingKeys_.end(), note->key) !=
                                 pendingKeys_.end() &&
                             std::fabs(note->start - groupStart_) < 0.08;

    Color color = t.track[(size_t)(note->track % 8)];
    float alpha = outOfRange ? 0.22f : 1.0f;
    if (practiceActive() && !target) alpha *= 0.32f;

    Rect body(column.x + 1.5f, topY, std::max(3.0f, column.w - 3.0f), height);

    if (active) {
      canvas_.roundRect(Rect(body.x - 3.0f, body.y - 2.0f, body.w + 6.0f, body.h + 4.0f),
                        body.w * 0.35f, color.withAlpha(0.22f * alpha));
    }

    canvas_.roundRect(body, std::min(5.0f, body.w * 0.3f), color.withAlpha(alpha));

    if (waitingHere) {
      canvas_.roundRect(body.inset(-1.5f).offset(0.0f, 0.0f), std::min(6.0f, body.w * 0.3f),
                        color.withAlpha(1.0f));
      canvas_.strokeRect(body.inset(-3.0f), std::min(7.0f, body.w * 0.3f),
                         t.keyHint.withAlpha(0.55f + 0.45f * (float)pulse), 1.6f);
    }

    if (settings_.noteNames && !outOfRange && body.h > 20.0f && body.w > 15.0f) {
      canvas_.text(noteName(note->key), body,
                   t.dark ? Color(0.06f, 0.07f, 0.12f, alpha) : Color(1, 1, 1, alpha), 10.5f,
                   Align::Center, VAlign::Middle, true);
    }
  }

  canvas_.popClip();

  // Out-of-window indicators -------------------------------------------
  auto drawRangePill = [&](const Rect& pill, const std::string& text, bool alarm) {
    const Color color = alarm ? t.warn : t.textMuted;
    canvas_.roundRect(pill, pill.h * 0.5f, t.panel.withAlpha(0.92f));
    canvas_.strokeRect(pill, pill.h * 0.5f, color.withAlpha(0.55f), 1.0f);
    canvas_.text(text, pill, color, 11.0f, Align::Center, VAlign::Middle, true, true);
  };

  if (anyAbove) {
    const int octaves = (highestOut - highKey + 11) / 12;
    drawRangePill(Rect(area.right() - 132.0f, area.y + 14.0f, 118.0f, 22.0f),
                  "8va +" + std::to_string(octaves), settings_.autoFollow);
  }
  if (anyBelow) {
    const int octaves = (lowKey - lowestOut + 11) / 12;
    drawRangePill(Rect(area.x + 14.0f, area.y + 14.0f, 118.0f, 22.0f),
                  "8vb -" + std::to_string(octaves), settings_.autoFollow);
  }

  // "Waiting for input" banner -----------------------------------------
  if (practiceActive() && waiting_ && !pendingKeys_.empty()) {
    std::string keys;
    for (size_t i = 0; i < pendingKeys_.size(); ++i) {
      if (i) keys += "  ";
      keys += noteName(pendingKeys_[i]);
    }
    const Rect banner(area.cx() - 130.0f, area.y + 12.0f, 260.0f, 34.0f);
    canvas_.roundRect(banner, 17.0f, t.keyHint.withAlpha(0.16f));
    canvas_.strokeRect(banner, 17.0f, t.keyHint.withAlpha(0.6f), 1.2f);
    canvas_.text(keys, banner, t.keyHint, 14.0f, Align::Center, VAlign::Middle, true, true);
  }
}

// ---------------------------------------------------------------------------
// Range overview (whole piano, current window, hardware coverage)
// ---------------------------------------------------------------------------

void App::drawRangeOverview(Rect area, bool interactive, bool big) {
  const Theme& t = theme();
  const int kLow = 21;
  const int kHigh = 108;

  int whites = 0;
  for (int key = kLow; key <= kHigh; ++key) {
    if (!isBlackKey(key)) ++whites;
  }
  if (whites <= 0 || area.w < 40.0f) return;

  const float whiteW = area.w / (float)whites;
  const float blackW = std::max(1.5f, whiteW * 0.62f);
  const float insetY = big ? 3.0f : 2.0f;

  auto keyLeft = [&](int key) -> float {
    int index = 0;
    for (int k = kLow; k < key; ++k) {
      if (!isBlackKey(k)) ++index;
    }
    return area.x + whiteW * (float)index - (isBlackKey(key) ? blackW * 0.5f : 0.0f);
  };
  auto keyRight = [&](int key) -> float {
    if (isBlackKey(key)) return keyLeft(key) + blackW;
    int index = 0;
    for (int k = kLow; k <= key; ++k) {
      if (!isBlackKey(k)) ++index;
    }
    return area.x + whiteW * (float)index;
  };
  auto keyCenter = [&](int key) -> float { return (keyLeft(key) + keyRight(key)) * 0.5f; };

  canvas_.roundRect(area, big ? 7.0f : 4.0f,
                    t.dark ? Color::hex(0x0E0E16) : Color::hex(0xFFFFFF));

  int index = 0;
  for (int key = kLow; key <= kHigh; ++key) {
    if (isBlackKey(key)) continue;
    canvas_.fill(Rect(area.x + whiteW * (float)index, area.y + insetY, whiteW - 0.4f,
                      area.h - insetY * 2.0f),
                 t.keyWhite.withAlpha(t.dark ? 0.18f : 0.9f));
    ++index;
  }
  index = 0;
  for (int key = kLow; key <= kHigh; ++key) {
    if (!isBlackKey(key)) {
      ++index;
      continue;
    }
    canvas_.fill(Rect(area.x + whiteW * (float)index - blackW * 0.5f, area.y + insetY, blackW,
                      (area.h - insetY * 2.0f) * 0.62f),
                 t.keyBlack.withAlpha(t.dark ? 0.85f : 0.7f));
  }

  // Current window -------------------------------------------------------
  const float windowX0 = std::max(area.x, keyLeft(windowLow_));
  const float windowX1 = std::min(area.right(), keyRight(windowHigh()));
  if (windowX1 > windowX0) {
    const Rect window(windowX0, area.y, windowX1 - windowX0, area.h);
    canvas_.roundRect(window, big ? 5.0f : 3.0f, t.accent.withAlpha(0.20f));
    canvas_.strokeRect(window, big ? 5.0f : 3.0f, t.accent, 1.4f);
  }

  // What the physical keys actually play --------------------------------
  const int hardwareLow = settings_.alignHardware ? windowLow_ : settings_.hardwareLow;
  const int hardwareHigh = std::min(108, hardwareLow + windowKeyCount() - 1);
  const float hardwareX0 = std::max(area.x, keyLeft(hardwareLow));
  const float hardwareX1 = std::min(area.right(), keyRight(hardwareHigh));
  if (hardwareX1 > hardwareX0) {
    const float barH = big ? 4.0f : 3.0f;
    canvas_.roundRect(Rect(hardwareX0, area.bottom() - barH - 1.0f, hardwareX1 - hardwareX0, barH),
                      barH * 0.5f, settings_.alignHardware ? t.good : t.warn);
  }

  // Notes about to play --------------------------------------------------
  if (!song_.empty()) {
    const double limit = time_ + (double)settings_.lookahead;
    auto first = std::lower_bound(
        song_.ordered.begin(), song_.ordered.end(), time_ - 1.5,
        [](const Note* note, double value) { return note->start < value; });
    for (auto it = first; it != song_.ordered.end(); ++it) {
      const Note* note = *it;
      if (note->start > limit) break;
      if (note->end() < time_ - 0.1) continue;
      if (!song_.tracks[(size_t)note->track].enabled) continue;
      const bool target = practiceActive() && isTargetNote(note);
      const Color color =
          target ? t.keyHint : t.track[(size_t)(note->track % 8)];
      canvas_.circle(keyCenter(note->key), area.y + area.h * 0.30f, big ? 2.3f : 1.7f,
                     color.withAlpha(0.95f));
    }
  }

  if (!interactive) return;

  // Drag anywhere on the strip to move the window.
  const Rect hitArea(area.x, area.y - 3.0f, area.w, area.h + 6.0f);
  if (canvas_.hitButton(hitArea)) scrubbingRange_ = true;

  if (scrubbingRange_) {
    if (canvas_.mouseHeld()) {
      int whiteIndex = (int)((canvas_.mouseX() - area.x) / std::max(1.0f, whiteW));
      whiteIndex = std::clamp(whiteIndex, 0, whites - 1);
      int targetKey = kLow;
      int seen = 0;
      for (int key = kLow; key <= kHigh; ++key) {
        if (isBlackKey(key)) continue;
        if (seen == whiteIndex) {
          targetKey = key;
          break;
        }
        ++seen;
      }
      const int count = windowKeyCount();
      const int maxLow = std::max(21, 108 - count + 1);
      windowLow_ = std::clamp(targetKey - count / 2, 21, maxLow);
    } else {
      scrubbingRange_ = false;
      settings_.keyLow = windowLow_;
      markSettingsDirty();
    }
  }
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

void App::drawKeyboard() {
  const Theme& t = theme();
  const Rect area = keyboardArea_;
  canvas_.fill(area, t.panel);
  canvas_.line(area.x, area.y, area.right(), area.y, t.border, 1.0f);

  // Overview strip: the whole piano with the current 32-key window marked.
  const bool showOverview = !song_.empty();
  float keyTop = area.y + 6.0f;
  if (showOverview) {
    drawRangeOverview(Rect(area.x + 10.0f, area.y + 5.0f, area.w - 20.0f, 20.0f), true, false);
    keyTop = area.y + 30.0f;
  }

  int whiteCount = 0;
  for (int key = windowLow_; key <= windowHigh(); ++key) {
    if (!isBlackKey(key)) ++whiteCount;
  }
  if (whiteCount <= 0) return;

  const float keyBottom = area.bottom() - 2.0f;
  const float whiteW = area.w / (float)whiteCount;
  const float blackW = whiteW * 0.62f;
  const float blackH = (keyBottom - keyTop) * 0.62f;

  int whiteIndex = 0;
  for (int key = windowLow_; key <= windowHigh(); ++key) {
    if (isBlackKey(key)) continue;
    const Rect r(area.x + whiteW * (float)whiteIndex, keyTop, whiteW - 0.5f, keyBottom - keyTop);
    whiteIndex++;

    const bool pressed = pressedKeys_[key];
    const bool autoPlayed = autoKeys_[key];
    Color base = t.keyWhite;
    if (autoPlayed) base = key < 60 ? t.keyActiveLeft : t.keyActiveRight;
    if (pressed) base = t.accent;
    canvas_.fill(r, base);
    // top shading
    canvas_.fill(Rect(r.x, r.y, r.w, 3.0f), t.keyWhiteTop.withAlpha(0.9f));
    canvas_.strokeRect(r, 0.0f, t.border, 1.0f);

    if (flash_[key] > 0.0f) {
      canvas_.fill(r, t.good.withAlpha((float)flash_[key] * 0.5f));
      flash_[key] = std::max(0.0f, (float)flash_[key] - 0.06f);
    } else if (flash_[key] < 0.0f) {
      canvas_.fill(r, t.bad.withAlpha((float)(-flash_[key]) * 0.5f));
      flash_[key] = std::min(0.0f, (float)flash_[key] + 0.06f);
    }

    if (settings_.noteNames && whiteW > 20.0f && key % 12 == 0) {
      canvas_.text(noteName(key), Rect(r.x, r.bottom() - 16.0f, r.w, 13.0f),
                   autoPlayed || pressed ? Color(1, 1, 1, 0.9f) : t.textMuted, 9.5f,
                   Align::Center, VAlign::Middle);
    }
  }

  whiteIndex = 0;
  for (int key = windowLow_; key <= windowHigh(); ++key) {
    if (!isBlackKey(key)) {
      ++whiteIndex;
      continue;
    }
    const Rect r(area.x + whiteW * (float)whiteIndex - blackW * 0.5f, keyTop, blackW, blackH);
    const bool pressed = pressedKeys_[key];
    const bool autoPlayed = autoKeys_[key];
    Color base = t.keyBlack;
    if (autoPlayed) base = key < 60 ? t.keyActiveLeft : t.keyActiveRight;
    if (pressed) base = t.accent;
    canvas_.roundRect(Rect(r.x, r.y, r.w, r.h), 3.0f, base);

    if (flash_[key] > 0.0f) {
      canvas_.roundRect(Rect(r.x, r.y, r.w, r.h), 3.0f, t.good.withAlpha((float)flash_[key] * 0.5f));
      flash_[key] = std::max(0.0f, (float)flash_[key] - 0.06f);
    } else if (flash_[key] < 0.0f) {
      canvas_.roundRect(Rect(r.x, r.y, r.w, r.h), 3.0f,
                        t.bad.withAlpha((float)(-flash_[key]) * 0.5f));
      flash_[key] = std::min(0.0f, (float)flash_[key] + 0.06f);
    }
  }

  // Mouse playing ------------------------------------------------------
  int hoveredKey = -1;
  for (int key = windowHigh(); key >= windowLow_; --key) {
    const bool black = isBlackKey(key);
    int idx = 0;
    for (int k = windowLow_; k < key; ++k) {
      if (!isBlackKey(k)) ++idx;
    }
    Rect r;
    if (black) {
      r = Rect(area.x + whiteW * (float)idx - blackW * 0.5f, keyTop, blackW, blackH);
    } else {
      r = Rect(area.x + whiteW * (float)idx, keyTop, whiteW - 0.5f, keyBottom - keyTop);
    }
    if (r.contains(canvas_.mouseX(), canvas_.mouseY())) {
      hoveredKey = key;
      break;
    }
  }

  if (canvas_.mouseHeld()) {
    if (hoveredKey >= 0 && hoveredKey != mouseKey_) {
      if (mouseKey_ >= 0) noteOffFromUser(mouseKey_);
      mouseKey_ = hoveredKey;
      noteOnFromUser(hoveredKey, 100);
    }
  } else if (mouseKey_ >= 0) {
    noteOffFromUser(mouseKey_);
    mouseKey_ = -1;
  }
}

// ---------------------------------------------------------------------------
// Practice panel
// ---------------------------------------------------------------------------

void App::drawPanel() {
  if (panelArea_.w <= 1.0f) return;
  const Theme& t = theme();
  const Rect panel = panelArea_;
  canvas_.fill(panel, t.panel);
  canvas_.line(panel.x, panel.y, panel.x, panel.bottom(), t.border, 1.0f);

  const float x = panel.x + 14.0f;
  const float w = panel.w - 28.0f;
  float y = panel.y + 12.0f;

  auto card = [&](Rect r, const char* title) {
    canvas_.roundRect(r, 11.0f, t.elevated);
    canvas_.strokeRect(r, 11.0f, t.border, 1.0f);
    canvas_.text(title, Rect(r.x + 14.0f, r.y + 8.0f, r.w - 28.0f, 18.0f), t.textMuted, 10.5f,
                 Align::Left, VAlign::Middle, true);
  };

  const bool practice = settings_.practiceMode;

  // ---- Practice card --------------------------------------------------
  float cardH = 46.0f + 32.0f;
  if (practice) cardH += 40.0f + (settings_.waitMode ? 42.0f : 0.0f) + 30.0f;
  const Rect practiceCard(x, y, w, cardH);
  card(practiceCard, str_->practice);

  float cy = practiceCard.y + 30.0f;
  int mode = settings_.practiceMode ? 1 : 0;
  const int beforeMode = mode;
  canvas_.segmented(10, Rect(x + 12.0f, cy, w - 24.0f, 30.0f),
                    {str_->listen, str_->practice}, mode, t);
  if (mode != beforeMode) {
    settings_.practiceMode = mode == 1;
    resetStats(false);
    resetCursorToTime();
    markSettingsDirty();
  }
  cy += 40.0f;

  if (practice) {
    canvas_.text(str_->handLabel, Rect(x + 14.0f, cy, 70.0f, 26.0f), t.textMuted, 11.0f,
                 Align::Left, VAlign::Middle);
    int hand = settings_.practiceHand;
    const int beforeHand = hand;
    canvas_.segmented(11, Rect(x + 84.0f, cy, w - 98.0f, 26.0f),
                      {str_->bothHands, str_->leftHand, str_->rightHand}, hand, t);
    if (hand != beforeHand) {
      settings_.practiceHand = hand;
      rebuildTargets();
      resetStats(false);
      resetCursorToTime();
      markSettingsDirty();
    }
    cy += 34.0f;

    bool wait = settings_.waitMode;
    if (canvas_.toggler(20, Rect(x + 12.0f, cy, w - 24.0f, 28.0f), str_->waitForInput, wait, t)) {
      settings_.waitMode = wait;
      markSettingsDirty();
    }
    cy += 30.0f;

    if (settings_.waitMode) {
      canvas_.text(str_->assist, Rect(x + 14.0f, cy, 64.0f, 28.0f), t.textMuted, 11.0f,
                   Align::Left, VAlign::Middle);
      int assist = settings_.assist;
      canvas_.dropdown(21, Rect(x + 78.0f, cy + 2.0f, w - 92.0f, 26.0f),
                       {str_->assistOff, str_->assistSlow, str_->assistFast}, assist, t);
      if (assist != settings_.assist) {
        settings_.assist = assist;
        markSettingsDirty();
      }
      cy += 38.0f;
    }

    bool autoSpeed = settings_.autoSpeed;
    if (canvas_.toggler(22, Rect(x + 12.0f, cy, w - 24.0f, 28.0f), str_->autoSpeed, autoSpeed, t)) {
      settings_.autoSpeed = autoSpeed;
      markSettingsDirty();
    }
  }

  y = practiceCard.bottom() + 10.0f;

  // ---- Speed + loop card ---------------------------------------------
  const Rect tempoCard(x, y, w, 142.0f);
  card(tempoCard, str_->speed);
  if (canvas_.slider(30, Rect(x + 10.0f, tempoCard.y + 24.0f, w - 20.0f, 26.0f), settings_.speed,
                     0.25f, 1.5f, t,
                     std::to_string((int)std::lround(settings_.speed * 100.0f)) + "%")) {
    markSettingsDirty();
  }

  bool loop = settings_.loop;
  if (canvas_.toggler(31, Rect(x + 12.0f, tempoCard.y + 56.0f, 120.0f, 26.0f), str_->loop, loop, t)) {
    settings_.loop = loop;
    markSettingsDirty();
  }

  if (settings_.loop) {
    int startBar = settings_.loopStart + 1;
    int endBar = settings_.loopEnd + 1;
    const int maxBar = std::max(1, song_.measureCount());
    canvas_.text(str_->loopFrom, Rect(x + 130.0f, tempoCard.y + 56.0f, 22.0f, 26.0f), t.textMuted,
                 11.0f, Align::Left, VAlign::Middle);
    if (canvas_.stepper(32, Rect(x + 152.0f, tempoCard.y + 56.0f, 62.0f, 26.0f), startBar, 1,
                        maxBar, t)) {
      settings_.loopStart = std::min(startBar - 1, settings_.loopEnd);
      markSettingsDirty();
    }
    canvas_.text(str_->loopTo, Rect(x + 218.0f, tempoCard.y + 56.0f, 22.0f, 26.0f), t.textMuted,
                 11.0f, Align::Left, VAlign::Middle);
    if (canvas_.stepper(33, Rect(x + 238.0f, tempoCard.y + 56.0f, 62.0f, 26.0f), endBar, 1,
                        maxBar, t)) {
      settings_.loopEnd = std::max(endBar - 1, settings_.loopStart);
      markSettingsDirty();
    }
  }

  bool metro = settings_.metronome;
  if (canvas_.toggler(34, Rect(x + 12.0f, tempoCard.y + 84.0f, w - 24.0f, 24.0f), str_->metronome,
                      metro, t)) {
    settings_.metronome = metro;
    markSettingsDirty();
  }

  // Visible keyboard window (for 25-49 key controllers)
  canvas_.text(str_->rangeLabel, Rect(x + 14.0f, tempoCard.y + 112.0f, 40.0f, 24.0f), t.textMuted,
               10.5f, Align::Left, VAlign::Middle);
  const std::string rangeText = noteName(windowLow_) + " - " + noteName(windowHigh());
  canvas_.text(rangeText, Rect(x + 56.0f, tempoCard.y + 112.0f, 116.0f, 24.0f), t.text, 11.5f,
               Align::Left, VAlign::Middle, false, true);
  const bool canShift = windowKeyCount() < 88;
  if (canvas_.button(35, Rect(tempoCard.right() - 94.0f, tempoCard.y + 111.0f, 38.0f, 24.0f), "-8",
                     t, false, canShift, 11.0f)) {
    shiftWindow(-1);
  }
  if (canvas_.button(36, Rect(tempoCard.right() - 50.0f, tempoCard.y + 111.0f, 38.0f, 24.0f), "+8",
                     t, false, canShift, 11.0f)) {
    shiftWindow(1);
  }
  y = tempoCard.bottom() + 10.0f;

  // ---- Stats card -----------------------------------------------------
  if (practice) {
    const Rect statsCard(x, y, w, 182.0f);
    card(statsCard, str_->accuracy);

    const int pct = (int)std::lround(accuracy());
    canvas_.text(std::to_string(pct) + "%", Rect(statsCard.x + 14.0f, statsCard.y + 24.0f, 120.0f, 40.0f),
                 pct >= 90 ? t.good : (pct >= 70 ? t.warn : t.bad), 30.0f, Align::Left,
                 VAlign::Middle, true);

    const std::string comboText = std::to_string(combo_);
    canvas_.text(str_->combo, Rect(statsCard.right() - 90.0f, statsCard.y + 26.0f, 76.0f, 14.0f),
                 t.textMuted, 10.0f, Align::Right, VAlign::Top);
    canvas_.text(comboText, Rect(statsCard.right() - 90.0f, statsCard.y + 38.0f, 76.0f, 22.0f),
                 t.text, 17.0f, Align::Right, VAlign::Middle, true);

    const float colW = (statsCard.w - 28.0f) / 3.0f;
    struct Stat {
      const char* label;
      int value;
      Color color;
    };
    const Stat stats[3] = {{str_->hits, hits_, t.good},
                           {str_->wrongNotes, wrong_, t.bad},
                           {str_->skipped, skipped_, t.warn}};
    for (int i = 0; i < 3; ++i) {
      const Rect cell(statsCard.x + 14.0f + colW * (float)i, statsCard.y + 72.0f, colW, 40.0f);
      canvas_.text(stats[i].label, Rect(cell.x, cell.y, cell.w, 14.0f), t.textMuted, 10.0f);
      canvas_.text(std::to_string(stats[i].value), Rect(cell.x, cell.y + 14.0f, cell.w, 20.0f),
                   stats[i].color, 15.0f, Align::Left, VAlign::Middle, true);
    }

    std::string targetText;
    if (!pendingKeys_.empty()) {
      for (size_t i = 0; i < pendingKeys_.size(); ++i) {
        if (i) targetText += " ";
        targetText += noteName(pendingKeys_[i]);
      }
    } else {
      targetText = "-";
    }
    canvas_.text(str_->targetKeys, Rect(statsCard.x + 14.0f, statsCard.y + 112.0f, 80.0f, 16.0f),
                 t.textMuted, 10.0f);
    canvas_.text(targetText, Rect(statsCard.x + 92.0f, statsCard.y + 110.0f,
                                  statsCard.w - 106.0f, 18.0f),
                 t.keyHint, 12.0f, Align::Left, VAlign::Middle, true, true);

    if (canvas_.button(40, Rect(statsCard.x + 12.0f, statsCard.y + 146.0f,
                                statsCard.w - 24.0f, 24.0f),
                       str_->resetStats, t, false, true, 11.0f)) {
      resetStats(false);
      resetCursorToTime();
    }
    y = statsCard.bottom() + 10.0f;
  }

  // ---- Tracks ---------------------------------------------------------
  const float tracksHeight = std::max(60.0f, panel.bottom() - y - 12.0f);
  const Rect tracksCard(x, y, w, tracksHeight);
  card(tracksCard, str_->tracks);

  canvas_.pushClip(Rect(tracksCard.x, tracksCard.y + 24.0f, tracksCard.w,
                        tracksCard.h - 28.0f));
  float rowY = tracksCard.y + 26.0f;
  for (int i = 0; i < (int)song_.tracks.size(); ++i) {
    if (rowY + 26.0f > tracksCard.bottom() - 4.0f) break;
    Track& track = song_.tracks[(size_t)i];
    const Rect row(tracksCard.x + 8.0f, rowY, tracksCard.w - 16.0f, 24.0f);
    canvas_.roundRect(Rect(row.x, row.y + 8.0f, 8.0f, 8.0f), 2.0f,
                      t.track[(size_t)(i % 8)].withAlpha(track.enabled ? 1.0f : 0.35f));
    std::string name = track.name.empty() ? ("Track " + std::to_string(i + 1)) : track.name;
    if (name.size() > 22) name = name.substr(0, 21) + "…";
    canvas_.text(name, Rect(row.x + 16.0f, row.y, row.w - 90.0f, row.h),
                 track.enabled ? t.textDim : t.textMuted, 11.5f);

    bool enabled = track.enabled;
    if (canvas_.toggler(50 + i, Rect(row.right() - 56.0f, row.y, 56.0f, row.h), "", enabled, t)) {
      track.enabled = enabled;
      rebuildTargets();
      resetCursorToTime();
      markSettingsDirty();
    }
    rowY += 28.0f;
  }
  canvas_.popClip();
}

// ---------------------------------------------------------------------------
// Empty state
// ---------------------------------------------------------------------------

void App::drawEmptyState() {
  const Theme& t = theme();
  const Rect area = notesArea_;
  canvas_.fill(area, t.bg);

  const float cardW = std::min(560.0f, area.w - 80.0f);
  const float cardH = 306.0f;
  const Rect card(area.cx() - cardW * 0.5f, area.cy() - cardH * 0.5f, cardW, cardH);

  canvas_.roundRect(card, 18.0f, t.panel);
  canvas_.strokeRect(card, 18.0f, t.border, 1.0f);

  drawAppMark(canvas_, Rect(card.cx() - 26.0f, card.y + 26.0f, 52.0f, 52.0f), t);

  canvas_.text(str_->noFileTitle, Rect(card.x + 20.0f, card.y + 92.0f, cardW - 40.0f, 32.0f),
               t.text, 19.0f, Align::Center, VAlign::Middle, true);
  canvas_.text(str_->noFileBody, Rect(card.x + 20.0f, card.y + 126.0f, cardW - 40.0f, 22.0f),
               t.textDim, 12.5f, Align::Center, VAlign::Middle);

  const float buttonW = 150.0f;
  const Rect openBtn(card.cx() - buttonW - 8.0f, card.y + 164.0f, buttonW, 40.0f);
  const Rect demoBtn(card.cx() + 8.0f, card.y + 164.0f, buttonW, 40.0f);
  if (canvas_.button(2, openBtn, str_->openMidi, t, true, true, 13.0f)) openFileDialog();
  if (canvas_.button(3, demoBtn, str_->demoTitle, t, false, true, 13.0f)) loadDemoSong();

  canvas_.text(str_->formatsHint, Rect(card.x + 20.0f, card.y + 220.0f, cardW - 40.0f, 20.0f),
               t.textMuted, 11.5f, Align::Center, VAlign::Middle);
  canvas_.text(str_->shortcutHint, Rect(card.x + 16.0f, card.y + 246.0f, cardW - 32.0f, 20.0f),
               t.textMuted, 10.5f, Align::Center, VAlign::Middle, false, true);
  canvas_.text(str_->keyboardHint, Rect(card.x + 16.0f, card.y + 270.0f, cardW - 32.0f, 20.0f),
               t.textMuted, 10.5f, Align::Center, VAlign::Middle);
}

// ---------------------------------------------------------------------------
// Settings page
// ---------------------------------------------------------------------------

void App::drawSettings() {
  const Theme& t = theme();
  const Rect full(0.0f, 0.0f, canvas_.width(), canvas_.height());
  canvas_.fill(full, t.bg);

  // Header
  const Rect header(0.0f, 0.0f, full.w, 72.0f);
  canvas_.fill(header, t.panel);
  canvas_.line(0.0f, header.bottom() - 1.0f, header.right(), header.bottom() - 1.0f, t.border, 1.0f);

  const Rect backBtn(20.0f, 20.0f, 92.0f, 32.0f);
  if (canvas_.button(100, backBtn, str_->back, t, false, true, 12.5f)) view_ = View::Player;

  canvas_.text(str_->settings, Rect(backBtn.right() + 18.0f, 0.0f, 300.0f, header.h), t.text,
               17.0f, Align::Left, VAlign::Middle, true);
  canvas_.text(str_->deviceHint, Rect(header.right() - 360.0f, 0.0f, 340.0f, header.h),
               t.textMuted, 11.5f, Align::Right, VAlign::Middle);

  const float margin = 32.0f;
  const float gap = 24.0f;
  const float colW = (full.w - margin * 2.0f - gap) * 0.5f;
  const float leftX = margin;
  const float rightX = margin + colW + gap;

  // Vertical scrolling (small windows still reach every card).
  const float wheel = (float)canvas_.wheelDelta() / 120.0f;
  if (std::fabs(wheel) > 0.001f) settingsScroll_ -= wheel * 72.0f;
  const float leftContent = 290.0f + gap + 116.0f + gap + 260.0f;
  const float rightContent = 176.0f + gap + 190.0f + gap + 92.0f;
  const float contentBottom = 96.0f + std::max(leftContent, rightContent) + 24.0f;
  const float maxScroll = std::max(0.0f, contentBottom - full.h);
  settingsScroll_ = std::clamp(settingsScroll_, 0.0f, maxScroll);

  float leftY = 96.0f - settingsScroll_;
  float rightY = 96.0f - settingsScroll_;

  auto panelBg = [&](Rect r) {
    canvas_.roundRect(r, 12.0f, t.panel);
    canvas_.strokeRect(r, 12.0f, t.border, 1.0f);
  };
  auto sectionTitle = [&](Rect card, const char* title) {
    canvas_.text(title, Rect(card.x + 16.0f, card.y + 10.0f, card.w - 32.0f, 20.0f), t.textDim,
                 12.0f, Align::Left, VAlign::Middle, true);
  };
  auto label = [&](float x, float y, float w, const char* text) {
    canvas_.text(text, Rect(x, y, w, 30.0f), t.textDim, 12.0f, Align::Left, VAlign::Middle);
  };

  // ---- MIDI input -----------------------------------------------------
  {
    const float cardH = 290.0f;
    const Rect cardRect(leftX, leftY, colW, cardH);
    panelBg(cardRect);
    sectionTitle(cardRect, str_->sectionMidiIn);

    const float labelW = 96.0f;
    const float controlX = cardRect.x + 16.0f + labelW;
    const float controlW = cardRect.w - labelW - 32.0f;
    float y = cardRect.y + 36.0f;

    label(cardRect.x + 16.0f, y, labelW, str_->inputDevice);
    std::vector<std::string> inputItems;
    inputItems.push_back(str_->noneOption);
    for (const auto& name : inputDevices_) inputItems.push_back(name);
    if (inputDevices_.empty()) inputItems.push_back(str_->noDevice);
    int inputIndex = std::clamp(settings_.midiInput, 0, (int)inputItems.size() - 1);
    canvas_.dropdown(110, Rect(controlX, y + 1.0f, controlW, 28.0f), inputItems, inputIndex, t);
    if (inputIndex != settings_.midiInput) {
      settings_.midiInput = inputIndex;
      openInputDevice();
      markSettingsDirty();
    }
    y += 40.0f;

    label(cardRect.x + 16.0f, y, labelW, str_->channel);
    std::vector<std::string> channelItems;
    channelItems.push_back(str_->allChannels);
    for (int i = 1; i <= 16; ++i) channelItems.push_back(std::to_string(i));
    int channel = std::clamp(settings_.midiChannel, 0, 16);
    canvas_.dropdown(111, Rect(controlX, y + 1.0f, controlW, 28.0f), channelItems, channel, t);
    if (channel != settings_.midiChannel) {
      settings_.midiChannel = channel;
      markSettingsDirty();
    }
    y += 40.0f;

    label(cardRect.x + 16.0f, y, labelW, str_->velocityCurve);
    int curve = settings_.velocityCurve;
    canvas_.dropdown(112, Rect(controlX, y + 1.0f, controlW, 28.0f),
                     {str_->curveLinear, str_->curveSoft, str_->curveHard, str_->curveFixed}, curve,
                     t);
    if (curve != settings_.velocityCurve) {
      settings_.velocityCurve = curve;
      markSettingsDirty();
    }
    y += 40.0f;

    label(cardRect.x + 16.0f, y, labelW, str_->transpose);
    int transpose = settings_.transpose;
    canvas_.stepper(113, Rect(controlX, y + 1.0f, 120.0f, 28.0f), transpose, -24, 24, t);
    if (transpose != settings_.transpose) {
      settings_.transpose = transpose;
      markSettingsDirty();
    }
    canvas_.text((transpose > 0 ? "+" : "") + std::to_string(transpose) + " st",
                 Rect(controlX + 126.0f, y, 90.0f, 30.0f), t.textMuted, 11.0f, Align::Left,
                 VAlign::Middle);
    y += 40.0f;

    label(cardRect.x + 16.0f, y, labelW, str_->latency);
    float latency = (float)settings_.latencyMs;
    if (canvas_.slider(114, Rect(controlX, y, controlW - 46.0f, 30.0f), latency, -150.0f, 150.0f,
                       t, std::to_string((int)settings_.latencyMs) + "ms")) {
      settings_.latencyMs = (int)std::lround(latency);
      markSettingsDirty();
    }
    y += 34.0f;
    canvas_.text(str_->latencyHint, Rect(cardRect.x + 16.0f, y, cardRect.w - 32.0f, 18.0f),
                 t.textMuted, 10.5f);

    const Rect refreshBtn(cardRect.x + 16.0f, cardRect.bottom() - 44.0f, 132.0f, 28.0f);
    if (canvas_.button(115, refreshBtn, str_->refresh, t, false, true, 11.5f)) {
      refreshDevices();
      openInputDevice();
      openOutputDevice();
    }
    const std::string status =
        midiIn_.isOpen() ? (std::string(str_->connected) + " · " + midiIn_.deviceName())
                         : std::string(str_->disconnected);
    canvas_.text(status, Rect(refreshBtn.right() + 12.0f, refreshBtn.y, cardRect.w - 190.0f, 28.0f),
                 midiIn_.isOpen() ? t.good : t.textMuted, 11.0f, Align::Left, VAlign::Middle);

    leftY += cardH + gap;
  }

  // ---- MIDI output ----------------------------------------------------
  {
    const Rect cardRect(leftX, leftY, colW, 116.0f);
    panelBg(cardRect);
    sectionTitle(cardRect, str_->sectionMidiOut);
    const float labelW = 96.0f;
    const float controlX = cardRect.x + 16.0f + labelW;
    const float controlW = cardRect.w - labelW - 32.0f;
    label(cardRect.x + 16.0f, cardRect.y + 36.0f, labelW, str_->outputDevice);
    std::vector<std::string> items;
    items.push_back(str_->noneOption);
    for (const auto& name : outputDevices_) items.push_back(name);
    if (outputDevices_.empty()) items.push_back(str_->noDevice);
    int index = std::clamp(settings_.midiOutput, 0, (int)items.size() - 1);
    canvas_.dropdown(120, Rect(controlX, cardRect.y + 37.0f, controlW, 28.0f), items, index, t);
    if (index != settings_.midiOutput) {
      settings_.midiOutput = index;
      openOutputDevice();
      markSettingsDirty();
    }
    canvas_.text(midiOut_.isOpen() ? str_->connected : str_->disconnected,
                 Rect(cardRect.x + 16.0f, cardRect.y + 76.0f, cardRect.w - 32.0f, 22.0f),
                 midiOut_.isOpen() ? t.good : t.textMuted, 11.0f);
    leftY += 116.0f + gap;
  }

  // ---- Keyboard range (small controllers) -----------------------------
  {
    const Rect cardRect(leftX, leftY, colW, 260.0f);
    panelBg(cardRect);
    sectionTitle(cardRect, str_->sectionKeyboard);

    const float labelW = 96.0f;
    const float controlX = cardRect.x + 16.0f + labelW;
    const float controlW = cardRect.w - labelW - 32.0f;
    float y = cardRect.y + 30.0f;

    label(cardRect.x + 16.0f, y, labelW, str_->keyCount);
    static const int kCounts[6] = {25, 32, 37, 49, 61, 88};
    std::vector<std::string> countItems;
    for (int value : kCounts) countItems.push_back(std::to_string(value));
    int countIndex = 5;
    for (int i = 0; i < 6; ++i) {
      if (kCounts[i] == settings_.keyCount) countIndex = i;
    }
    const int beforeCount = countIndex;
    canvas_.segmented(160, Rect(controlX, y, controlW, 28.0f), countItems, countIndex, t);
    if (countIndex != beforeCount) {
      settings_.keyCount = kCounts[countIndex];
      resetKeyboardWindow();
      rebuildTargets();
      resetCursorToTime();
      markSettingsDirty();
    }
    y += 36.0f;

    // Whole-piano diagram: drag it to choose exactly which keys you get.
    const Rect diagram(cardRect.x + 16.0f, y, cardRect.w - 32.0f, 44.0f);
    drawRangeOverview(diagram, true, true);
    y += 50.0f;

    const std::string windowText = std::string(str_->windowLabel) + " " + noteName(windowLow_) +
                                   " - " + noteName(windowHigh());
    canvas_.text(windowText, Rect(diagram.x, y, diagram.w * 0.5f, 18.0f), t.accent, 11.0f,
                 Align::Left, VAlign::Middle, false, true);

    const int hardwareLow = settings_.alignHardware ? windowLow_ : settings_.hardwareLow;
    const int hardwareHigh = std::min(108, hardwareLow + windowKeyCount() - 1);
    const std::string hardwareText = std::string(str_->hardwareLabel) + " " +
                                     noteName(hardwareLow) + " - " + noteName(hardwareHigh);
    canvas_.text(hardwareText, Rect(diagram.x + diagram.w * 0.5f, y, diagram.w * 0.5f, 18.0f),
                 settings_.alignHardware ? t.good : t.warn, 11.0f, Align::Right, VAlign::Middle,
                 false, true);
    y += 24.0f;

    bool follow = settings_.autoFollow;
    if (canvas_.toggler(162, Rect(cardRect.x + 16.0f, y, cardRect.w * 0.5f - 24.0f, 28.0f),
                        str_->autoFollow, follow, t)) {
      settings_.autoFollow = follow;
      markSettingsDirty();
    }
    bool align = settings_.alignHardware;
    if (canvas_.toggler(163, Rect(cardRect.x + cardRect.w * 0.5f, y, cardRect.w * 0.5f - 24.0f, 28.0f),
                        str_->alignHardware, align, t)) {
      settings_.alignHardware = align;
      markSettingsDirty();
    }
    y += 30.0f;

    bool accomp = settings_.outOfRangeAccomp;
    if (canvas_.toggler(164, Rect(cardRect.x + 16.0f, y, cardRect.w - 32.0f, 28.0f),
                        str_->outOfRangeAccomp, accomp, t)) {
      settings_.outOfRangeAccomp = accomp;
      rebuildTargets();
      resetCursorToTime();
      markSettingsDirty();
    }
    y += 30.0f;

    label(cardRect.x + 16.0f, y, labelW, str_->hardwareLow);
    int hwLowSetting = settings_.hardwareLow;
    if (canvas_.stepper(165, Rect(controlX, y, 118.0f, 28.0f), hwLowSetting, 0, 108, t)) {
      settings_.hardwareLow = hwLowSetting;
      markSettingsDirty();
    }
    canvas_.text(align ? noteName(settings_.hardwareLow) : std::string("-"),
                 Rect(controlX + 126.0f, y, 150.0f, 28.0f), align ? t.text : t.textMuted, 12.0f,
                 Align::Left, VAlign::Middle, false, true);
    y += 32.0f;

    canvas_.textWrapped(str_->keyboardRangeHint, Rect(cardRect.x + 16.0f, y, cardRect.w - 32.0f, 36.0f),
                        t.textMuted, 10.5f);

    leftY += 260.0f + gap;
  }

  // ---- Audio ----------------------------------------------------------
  {
    const Rect cardRect(rightX, rightY, colW, 176.0f);
    panelBg(cardRect);
    sectionTitle(cardRect, str_->sectionAudio);
    const float labelW = 96.0f;
    const float controlX = cardRect.x + 16.0f + labelW;
    const float controlW = cardRect.w - labelW - 32.0f;

    label(cardRect.x + 16.0f, cardRect.y + 36.0f, labelW, str_->masterVolume);
    if (canvas_.slider(130, Rect(controlX, cardRect.y + 36.0f, controlW - 40.0f, 30.0f),
                       settings_.volume, 0.0f, 1.0f, t,
                       std::to_string((int)std::lround(settings_.volume * 100.0f)) + "%")) {
      markSettingsDirty();
    }

    label(cardRect.x + 16.0f, cardRect.y + 78.0f, labelW, str_->metronomeVolume);
    if (canvas_.slider(131, Rect(controlX, cardRect.y + 78.0f, controlW - 40.0f, 30.0f),
                       settings_.metronomeVolume, 0.0f, 1.0f, t,
                       std::to_string((int)std::lround(settings_.metronomeVolume * 100.0f)) + "%")) {
      markSettingsDirty();
    }

    canvas_.text(str_->audioEngine, Rect(cardRect.x + 16.0f, cardRect.y + 122.0f, labelW, 24.0f),
                 t.textMuted, 11.0f, Align::Left, VAlign::Middle);
    canvas_.text(audioStatus_.empty() ? str_->audioStopped : audioStatus_,
                 Rect(controlX, cardRect.y + 122.0f, controlW, 24.0f),
                 audioOk_ ? t.good : t.bad, 11.0f, Align::Left, VAlign::Middle);
    rightY += 176.0f + gap;
  }

  // ---- Display --------------------------------------------------------
  {
    const Rect cardRect(rightX, rightY, colW, 190.0f);
    panelBg(cardRect);
    sectionTitle(cardRect, str_->sectionDisplay);
    const float labelW = 96.0f;
    const float controlX = cardRect.x + 16.0f + labelW;
    const float controlW = cardRect.w - labelW - 32.0f;

    label(cardRect.x + 16.0f, cardRect.y + 36.0f, labelW, str_->theme);
    int themeIndex = settings_.theme;
    const int beforeTheme = themeIndex;
    canvas_.segmented(140, Rect(controlX, cardRect.y + 37.0f, controlW, 28.0f),
                      {str_->themeDark, str_->themeLight}, themeIndex, t);
    if (themeIndex != beforeTheme) {
      settings_.theme = themeIndex;
      markSettingsDirty();
    }

    bool noteNames = settings_.noteNames;
    if (canvas_.toggler(141, Rect(cardRect.x + 16.0f, cardRect.y + 80.0f, cardRect.w - 32.0f, 28.0f),
                        str_->noteNames, noteNames, t)) {
      settings_.noteNames = noteNames;
      markSettingsDirty();
    }

    label(cardRect.x + 16.0f, cardRect.y + 116.0f, labelW, str_->lookahead);
    float lookahead = (float)settings_.lookahead;
    if (canvas_.slider(142, Rect(controlX, cardRect.y + 116.0f, controlW - 40.0f, 30.0f), lookahead,
                       1.0f, 6.0f, t,
                       std::to_string((int)settings_.lookahead) + "s")) {
      settings_.lookahead = (int)std::lround(lookahead);
      markSettingsDirty();
    }
    canvas_.text(str_->lookaheadHint, Rect(cardRect.x + 16.0f, cardRect.y + 152.0f,
                                           cardRect.w - 32.0f, 18.0f),
                 t.textMuted, 10.5f);
    rightY += 190.0f + gap;
  }

  // ---- Language -------------------------------------------------------
  {
    const Rect cardRect(rightX, rightY, colW, 92.0f);
    panelBg(cardRect);
    sectionTitle(cardRect, str_->language);
    const float labelW = 96.0f;
    const float controlX = cardRect.x + 16.0f + labelW;
    const float controlW = cardRect.w - labelW - 32.0f;
    int language = settings_.language;
    const int beforeLanguage = language;
    canvas_.segmented(150, Rect(controlX, cardRect.y + 38.0f, controlW, 30.0f),
                      {str_->chinese, str_->english}, language, t);
    if (language != beforeLanguage) {
      settings_.language = language;
      markSettingsDirty();
    }
    rightY += 92.0f + gap;
  }

  // Scrollbar (only when the cards overflow the window) ------------------
  if (maxScroll > 1.0f) {
    const float trackX = full.w - 12.0f;
    const float trackY = 84.0f;
    const float trackH = full.h - 100.0f;
    canvas_.roundRect(Rect(trackX, trackY, 4.0f, trackH), 2.0f, t.elevated);
    const float contentH = contentBottom - 96.0f;
    const float thumbH = std::max(44.0f, trackH * (full.h / std::max(1.0f, contentH)));
    const float thumbY = trackY + (trackH - thumbH) * (settingsScroll_ / maxScroll);
    canvas_.roundRect(Rect(trackX, thumbY, 4.0f, thumbH), 2.0f, t.textMuted);
  }
}

} // namespace nr
