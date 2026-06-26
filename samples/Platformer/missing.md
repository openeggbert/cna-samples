# Missing / Differences from XNA 4.0 original

## Accelerometer input removed
**XNA behaviour:** Player movement can be controlled via accelerometer on phone.
**CNA port behaviour:** Accelerometer input is absent; keyboard and gamepad still work.
**Root cause:** CNA has no Accelerometer API.
**Tracked in:** not planned (desktop target only).

## Touch input removed
**XNA behaviour:** Touch screen input triggers jump and `continuePressed`.
**CNA port behaviour:** Only keyboard (Space/Up/W) and gamepad (A) trigger jump/continue.
**Root cause:** CNA has no TouchPanel API.
**Tracked in:** not planned.

## HUD font changed
**XNA behaviour:** Uses Pericles Regular 14pt SpriteFont.
**CNA port behaviour:** Uses DejaVu Sans 19px generated via make_font.py.
**Root cause:** XNA .xnb SpriteFont files are not supported; TTF substitution used instead.
**Tracked in:** DEFERRED.md (SpriteFont generation tooling).

## Sound files converted from WMA
**XNA behaviour:** Sound effects loaded from .wma files via XNA Content Pipeline.
**CNA port behaviour:** Sound effects are pre-converted to .wav; Music.wma kept as-is (Song supports .wma).
**Root cause:** SoundEffect only supports .wav in CNA.
**Tracked in:** not planned.
