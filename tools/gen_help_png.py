#!/usr/bin/env python3
"""
gen_help_png.py — generate a help overlay PNG from an XNA sample .htm file.

Usage:
    python3 tools/gen_help_png.py <sample.htm> <out/Content/help.png>

The script extracts the "Sample Controls" table from the official XNA 4.0
sample documentation and renders it as a PNG suitable for use as a help
overlay texture (F1 key) in CNA C++ ports.

The PNG has a semi-transparent white background baked in (RGBA).
Requires ImageMagick (convert) and DejaVu fonts.
"""

import sys
import re
import subprocess
import os


def strip_tags(s):
    return re.sub(r'<[^>]+>', '', s)


def decode_entities(s):
    for ent, ch in [('&gt;', '>'), ('&lt;', '<'), ('&amp;', '&'),
                    ('&nbsp;', ' '), ('&#160;', ' '), ('&quot;', '"'),
                    ('&#39;', "'")]:
        s = s.replace(ent, ch)
    return s


def clean(s):
    return decode_entities(strip_tags(s)).strip()


def extract_controls(htm_path):
    with open(htm_path, encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Find "Sample Controls" section (ends at next </div>)
    m = re.search(r'Sample Controls.*?</div>', content, re.DOTALL | re.IGNORECASE)
    if not m:
        return []

    section = m.group(0)

    # Extract table rows
    rows = re.findall(r'<tr[^>]*>(.*?)</tr>', section, re.DOTALL | re.IGNORECASE)

    controls = []
    for row in rows:
        cells = [clean(c) for c in re.findall(r'<t[hd][^>]*>(.*?)</t[hd]>', row, re.DOTALL | re.IGNORECASE)]
        if not cells:
            continue
        # Skip header rows
        if any('control' in c.lower() for c in cells):
            continue
        # Expect at least Action + Keyboard columns
        if len(cells) >= 2:
            action   = cells[0]
            keyboard = cells[1]
            if action and keyboard:
                controls.append((action, keyboard))

    return controls


def build_text(controls, sample_name):
    lines = []
    lines.append(f'=== {sample_name} — Controls ===')
    lines.append('')
    if controls:
        # Find longest action for alignment
        max_a = max(len(a) for a, _ in controls)
        for action, keyboard in controls:
            lines.append(f'  {action:<{max_a}}  {keyboard}')
    else:
        lines.append('  (No controls documented)')
    lines.append('')
    lines.append('  F1       Show / hide this help (10 s)')
    lines.append('  ESC      Exit')
    return '\n'.join(lines)


def render_png(text, out_path):
    # Render with ImageMagick:
    #   1. pango: text on transparent background
    #   2. composite over semi-transparent white rectangle
    #   3. force RGBA (color-type 6) so CNA/STB loads it with alpha channel
    escaped = text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    pango = f'<span font_family="monospace" size="13000">{escaped}</span>'

    text_png = out_path + '.text_tmp.png'
    bg_png   = out_path + '.bg_tmp.png'
    try:
        # Step 1: render text on transparent bg
        subprocess.run([
            'convert', '-background', 'none', '-fill', 'black',
            f'pango:{pango}', '-bordercolor', 'none', '-border', '16x16',
            text_png,
        ], check=True)

        # Step 2: get dimensions
        r = subprocess.run(['identify', '-format', '%wx%h', text_png],
                           capture_output=True, text=True, check=True)
        dims = r.stdout.strip()

        # Step 3: white semi-transparent background (alpha ≈ 80%)
        subprocess.run([
            'convert', '-size', dims, 'xc:rgba(255,255,255,200)',
            '-alpha', 'set', '-channel', 'Alpha', '-evaluate', 'set', '78%',
            bg_png,
        ], check=True)

        # Step 4: composite text over bg, force RGBA PNG
        subprocess.run([
            'convert', bg_png, text_png, '-composite',
            '-define', 'png:color-type=6',
            out_path,
        ], check=True)

        print(f'Written: {out_path}  ({dims})')
    finally:
        for p in (text_png, bg_png):
            if os.path.exists(p):
                os.unlink(p)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)

    htm_path, out_path = sys.argv[1], sys.argv[2]
    sample_name = os.path.splitext(os.path.basename(htm_path))[0]

    controls = extract_controls(htm_path)
    text = build_text(controls, sample_name)
    print('Extracted text:\n' + text + '\n')

    os.makedirs(os.path.dirname(out_path) or '.', exist_ok=True)
    render_png(text, out_path)


if __name__ == '__main__':
    main()
