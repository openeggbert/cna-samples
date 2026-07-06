# Missing / Differences from XNA 4.0 original

## Color key applied manually to assets
**XNA behaviour:** The XNA Content Pipeline automatically converts magenta (#FF00FF)
pixels to transparent (A=0) when processing BMP textures via color keying.
**CNA port behaviour:** The BMP files were converted to PNG with magenta replaced by
transparent pixels using ImageMagick before inclusion in the project.
**Root cause:** CNA does not use the XNA Content Pipeline; assets must be in an open
format with transparency pre-applied.
**Tracked in:** not planned.

## Color named constants replaced with RGBA literals
**XNA behaviour:** Uses `Color.Red`, `Color.CornflowerBlue`, `Color.White`.
**CNA port behaviour:** Equivalent RGBA values (e.g. `Color(255, 0, 0, 255)`) used
directly instead of `Color::Red` / `Color::CornflowerBlue` / `Color::White`.
**Root cause:** Stylistic leftover from before CNA added named color constants;
CNA's `Color` class now exposes `Color::Red`, `Color::CornflowerBlue`,
`Color::White`, etc. (see `Color.hpp`), so this is no longer a framework
limitation, just unmigrated port code. Behaviourally identical either way.
**Tracked in:** not planned (cosmetic only).

## F1 help overlay shows no documented controls
**XNA behaviour:** N/A — the F1 help overlay is a CNA-only addition with no XNA
equivalent.
**CNA port behaviour:** The generated `Content/help.png` overlay displays only
`(No controls documented)` plus the generic `F1`/`ESC` lines, instead of the game's
actual controls (Left/Right arrow keys or D-pad to move, Escape/Back to exit).
**Root cause:** `tools/gen_help_png.py` extracts the "Sample Controls" table from the
sample's `.htm` documentation, but `PerPixelCollision.htm` is a step-by-step tutorial
page (Collision Series 2) that has no "Sample Controls" section, so the extractor
correctly finds nothing rather than fabricating one. The other two Collision Series
tutorial samples (RectangleCollision, TransformedCollision) have the same gap for the
same reason.
**Tracked in:** not planned (asset-generation limitation, not a CNA framework gap).
