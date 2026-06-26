# Missing / Differences from XNA 4.0 original

## Accelerometer and touch input omitted
**XNA behaviour:** The original targets Windows Phone 7; accelerometer tilt steers the
player and touch-screen taps trigger jump / continue.
**CNA port behaviour:** Desktop-only port; keyboard (←/→/A/D to move, Space/↑/W to jump)
and gamepad (D-Pad + A) are the only inputs.
**Root cause:** Phone-specific features outside the scope of a desktop port.
**Tracked in:** not planned.

## HUD font changed
**XNA behaviour:** Uses Pericles Regular 14pt SpriteFont (.xnb).
**CNA port behaviour:** Uses DejaVu Sans 19px atlas generated with make_font.py.
**Root cause:** XNA .xnb binaries are not supported; open TTF substituted.
**Tracked in:** DEFERRED.md (SpriteFont tooling).

## Sound files converted from WMA
**XNA behaviour:** Sound effects shipped as .wma; loaded by the XNA Content Pipeline.
**CNA port behaviour:** .wma files pre-converted to .wav with ffmpeg; Music.wma kept
as-is because Song supports WMA directly.
**Root cause:** SoundEffect only supports .wav in CNA.
**Tracked in:** not planned.
