#!/usr/bin/env python3
"""Generate the desktop app icons from the NoteRain logo.

The source logo is a horizontal lockup (mark + wordmark). For an app icon we
only want the mark, cropped to its bounding box, padded to a square on a white
rounded background.

Usage:  python desktop/scripts/make-icon.py
Requires Pillow (bundled with the DSH python runtime).
"""

from __future__ import annotations

import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(ROOT, "client", "public", "logo.png")
OUT_DIR = os.path.join(ROOT, "desktop", "build")

ICON_SIZES = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]


def flatten(image: Image.Image, background=(255, 255, 255)) -> Image.Image:
    """Composite an RGBA image over a solid background."""
    rgba = image.convert("RGBA")
    canvas = Image.new("RGBA", rgba.size, background + (255,))
    canvas.alpha_composite(rgba)
    return canvas.convert("RGB")


def mark_bounding_box(image: Image.Image, gap: int = 12):
    """Bounding box of the mark that precedes the wordmark.

    Walks the column ink profile from the left, keeps the first connected block
    of ink and stops at the first sufficiently wide empty gap 鈥?that gap is the
    space between the mark and the "NoteRain" wordmark.
    """
    grey = image.convert("L")
    width, height = grey.size
    mask = grey.point(lambda value: 255 if value < 235 else 0)
    columns = []
    for x in range(width):
        column = mask.crop((x, 0, x + 1, height))
        columns.append(column.getbbox() is not None)

    start = next((x for x, has_ink in enumerate(columns) if has_ink), 0)
    end = start
    empty_run = 0
    for x in range(start, width):
        if columns[x]:
            end = x
            empty_run = 0
        else:
            empty_run += 1
            if empty_run >= gap:
                break

    left_band = mask.crop((start, 0, end + 1, height))
    band_box = left_band.getbbox() or (0, 0, end - start + 1, height)
    return (start + band_box[0], band_box[1], start + band_box[2], band_box[3])


def build_icon() -> Image.Image:
    source = flatten(Image.open(SRC))
    box = mark_bounding_box(source)
    mark = source.crop(box)

    # Square canvas with breathing room, mark centred.
    side = int(max(mark.size) * 1.34)
    canvas = Image.new("RGB", (side, side), (255, 255, 255))
    canvas.paste(mark, ((side - mark.size[0]) // 2, (side - mark.size[1]) // 2))

    icon = canvas.resize((512, 512), Image.LANCZOS)

    # Rounded corners (Windows/macOS style) with transparent outside.
    radius = int(512 * 0.165)
    mask = Image.new("L", (512, 512), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, 511, 511), radius=radius, fill=255)
    rounded = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    rounded.paste(icon, (0, 0), mask)
    return rounded


def main() -> None:
    os.makedirs(OUT_DIR, exist_ok=True)
    icon = build_icon()

    png_path = os.path.join(OUT_DIR, "icon.png")
    ico_path = os.path.join(OUT_DIR, "icon.ico")
    icon.save(png_path)
    icon.save(ico_path, sizes=ICON_SIZES)

    print(f"wrote {png_path} ({os.path.getsize(png_path)} bytes)")
    print(f"wrote {ico_path} ({os.path.getsize(ico_path)} bytes)")


if __name__ == "__main__":
    main()
