#!/usr/bin/env python3
"""
make_font.py  —  Generate a CNA SpriteFont asset from a TrueType font.

Usage:
    python3 make_font.py <ttf_path> <size_px> <output_base>
                         [--chars CHARS] [--padding N]

Outputs:
    <output_base>.font.json   — CNA font descriptor
    <output_base>.png         — glyph atlas (RGBA, power-of-two)

The .font.json format is exactly what CNA's SpriteFontTypeReader expects:
    {
        "texture": "<output_base>.png",
        "lineSpacing": <int>,
        "spacing": <float>,
        "defaultCharacter": "?",
        "glyphs": [
            {
                "char": <int>,           // Unicode code point
                "source": [x, y, w, h], // atlas rectangle (ink bounding box)
                "crop": [0, top, w, h], // top = distance from ascent line to ink top
                "kerning": [l, a, r]    // leftBearing, glyphWidth, rightBearing
            },
            ...
        ]
    }

Kerning semantics (must match SpriteBatch::DrawString logic):
    kern.X (l) = left bearing  — horizontal offset before ink begins
    kern.Y (a) = ink width     — pixel width of the drawn glyph
    kern.Z (r) = right bearing — gap after ink before next glyph
    advance = kern.X + kern.Y + kern.Z
"""

import argparse
import json
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont


# Printable ASCII + Latin-1 supplement (covers most European languages)
DEFAULT_CHARS = (
    "".join(chr(c) for c in range(32, 127))   # space..~
    + "".join(chr(c) for c in range(160, 256)) # NBSP..ÿ
)


def next_power_of_two(n: int) -> int:
    return 1 << math.ceil(math.log2(max(n, 1)))


def generate_font(ttf_path: str, size_px: int, output_base: str,
                  charset: str, padding: int) -> None:
    font = ImageFont.truetype(ttf_path, size_px)
    ascent, descent = font.getmetrics()
    line_spacing = ascent + descent

    # --- Measure all glyphs (including space) --------------------------------
    glyphs = []  # list of dicts filled below

    for ch in charset:
        cp = ord(ch)
        bbox = font.getbbox(ch)   # (left, top, right, bottom) relative to draw origin
        advance = int(round(font.getlength(ch)))

        if bbox is None or ch == ' ':
            # Space: no ink.  Advance carries the full width.
            glyphs.append({
                "char": cp,
                "atlas_x": 0, "atlas_y": 0, "w": 0, "h": 0,
                "crop_top": 0,
                "kern_x": 0.0,
                "kern_y": float(advance),
                "kern_z": 0.0,
                "has_ink": False,
            })
            continue

        left, top, right, bottom = bbox
        ink_w = right - left
        ink_h = bottom - top

        # left bearing / right bearing
        kern_x = float(left)
        kern_y = float(ink_w)
        kern_z = float(max(0, advance - right))

        glyphs.append({
            "char": cp,
            "atlas_x": 0, "atlas_y": 0,   # filled in packing step
            "w": ink_w, "h": ink_h,
            "crop_top": top,               # vertical offset from ascent line
            "kern_x": kern_x,
            "kern_y": kern_y,
            "kern_z": kern_z,
            "has_ink": True,
        })

    # --- Pack glyphs into an atlas -------------------------------------------
    # Row-based left-to-right packing; start with a guess width = 512.
    # If glyphs overflow, double the width (up to 4096 max).

    def pack(atlas_w: int):
        x, y, row_h = padding, padding, 0
        for g in glyphs:
            if not g["has_ink"]:
                continue
            if x + g["w"] + padding > atlas_w:
                y += row_h + padding
                x = padding
                row_h = 0
            g["atlas_x"] = x
            g["atlas_y"] = y
            x += g["w"] + padding
            row_h = max(row_h, g["h"])
        total_h = y + row_h + padding
        return total_h

    atlas_w = 512
    while True:
        total_h = pack(atlas_w)
        atlas_h = next_power_of_two(total_h)
        if atlas_h <= atlas_w * 2:
            break
        atlas_w = min(atlas_w * 2, 4096)

    atlas_w = next_power_of_two(atlas_w)

    # --- Render glyphs onto atlas --------------------------------------------
    atlas = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(atlas)

    for g in glyphs:
        if not g["has_ink"]:
            continue
        # getbbox() returns the bounding box of the ink relative to the draw origin.
        # To place ink at (atlas_x, atlas_y) we shift the draw origin by (-left, -top),
        # i.e., we draw at (atlas_x - left, atlas_y - top).
        left = int(g["kern_x"])  # left bearing = left in bbox
        top  = g["crop_top"]
        draw.text((g["atlas_x"] - left, g["atlas_y"] - top),
                  chr(g["char"]), font=font, fill=(255, 255, 255, 255))

    # --- Write atlas PNG -----------------------------------------------------
    atlas_filename = os.path.basename(output_base) + ".png"
    atlas_path = output_base + ".png"
    atlas.save(atlas_path)
    print(f"Atlas: {atlas_path}  ({atlas_w}×{atlas_h})")

    # --- Build .font.json ----------------------------------------------------
    font_json = {
        "texture": atlas_filename,
        "lineSpacing": line_spacing,
        "spacing": 0.0,
        "defaultCharacter": "?",
        "glyphs": [],
    }

    for g in glyphs:
        entry = {
            "char": g["char"],
            "source": [g["atlas_x"], g["atlas_y"], g["w"], g["h"]],
            "crop":   [0, g["crop_top"], g["w"], g["h"]],
            "kerning": [g["kern_x"], g["kern_y"], g["kern_z"]],
        }
        font_json["glyphs"].append(entry)

    json_path = output_base + ".font.json"
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(font_json, f, indent=2, ensure_ascii=False)
    print(f"Font:  {json_path}  ({len(glyphs)} glyphs)")


def main():
    parser = argparse.ArgumentParser(description="Generate CNA SpriteFont from TTF")
    parser.add_argument("ttf", help="Path to .ttf file")
    parser.add_argument("size", type=int, help="Font size in pixels")
    parser.add_argument("output", help="Output base name (no extension)")
    parser.add_argument("--chars", default=None,
                        help="Character set string (default: printable ASCII + Latin-1)")
    parser.add_argument("--padding", type=int, default=1,
                        help="Pixel padding between glyphs in atlas (default: 1)")
    args = parser.parse_args()

    charset = args.chars if args.chars is not None else DEFAULT_CHARS
    generate_font(args.ttf, args.size, args.output, charset, args.padding)


if __name__ == "__main__":
    main()
