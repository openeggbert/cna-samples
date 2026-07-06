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

## Sound effects converted from WMA
**XNA behaviour:** Sound effects (ExitReached, GemCollected, MonsterKilled, PlayerFall,
PlayerJump, PlayerKilled, Powerup) shipped as .wma; loaded by the XNA Content Pipeline.
**CNA port behaviour:** .wma files pre-converted to .wav with ffmpeg and play correctly.
**Root cause:** SoundEffect only supports .wav in CNA.
**Tracked in:** not planned.

## Background music never plays (WMA unsupported by CNA's audio backend)
**XNA behaviour:** `Sounds/Music.wma` loops continuously in the background via
`MediaPlayer.Play`/`MediaPlayer.IsRepeating`.
**CNA port behaviour:** `Content/Sounds/Music.wma` was left as the original ASF/WMA
file (byte-identical to the XNA original) instead of being converted. CNA's audio
backend (SDL3_mixer) only ships WAV, Vorbis (OGG), FLAC, VOC, AIFF, AU, and MP3
decoders — no WMA/ASF decoder. Verified experimentally against the exact SDL3_mixer
build this project links: `MIX_LoadAudio` on this file fails with "Audio data is in
unknown/unsupported/corrupt format". `MediaPlayer`'s internal `PlaySong` helper
returns early when that call fails, so the game runs normally but background music
never audibly plays. The `try/catch` around `MediaPlayer::Play(...)` in
`PlatformerGame::LoadContent()` never fires (no exception is thrown; the load just
silently fails), so this was previously miscategorized in this file as "working
because Song supports WMA directly" — it does not.
**Root cause:** CNA/SDL3_mixer has no WMA decoder; `Music.wma` should have been
re-encoded to `.ogg` or `.wav` per CLAUDE.md's Assets conversion guidance, like the
sound effects were, but was not.
**Tracked in:** not planned (re-encoding `Music.wma` to `.ogg`/`.wav` would fix this).

## Level file line-length validation omitted
**XNA behaviour:** `Level.LoadTiles` reads every line of the level `.txt` file and
throws an `Exception` naming the offending line number if any line's length differs
from the first line's width.
**CNA port behaviour:** `Level::LoadTiles` takes `lines[0].size()` as the width and
does not verify that every subsequent line has the same length; a malformed level
file with a short line would read out of bounds instead of throwing a clear error.
**Root cause:** Simplification made when porting the tile-loading loop; all shipped
level files are well-formed so this path is never exercised in practice.
**Tracked in:** not planned.
