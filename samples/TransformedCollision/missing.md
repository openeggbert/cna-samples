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
**CNA port behaviour:** Equivalent RGBA values used directly (e.g. `Color(255, 0, 0, 255)`).
**Root cause:** Stylistic port choice only — CNA's `Color` class now exposes named
static constants (`Color::Red`, `Color::CornflowerBlue`, `Color::White`, etc., see
`Color.hpp`), but the port predates that addition and was never updated to use them.
**Tracked in:** not planned (cosmetic only; behavior is identical).

## F1 help overlay shows no documented controls
**XNA behaviour:** N/A — the F1 help overlay is a CNA-only addition with no XNA
equivalent.
**CNA port behaviour:** The generated `Content/help.png` overlay displays only
`(No controls documented)` plus the generic `F1`/`ESC` lines, instead of the game's
actual controls (Left/Right arrow keys or D-pad to move, Escape/Back to exit).
**Root cause:** `tools/gen_help_png.py` extracts the "Sample Controls" table from the
sample's `.htm` documentation, but `TransformedCollision.htm` is a step-by-step
tutorial page (Collision Series 3) that has no "Sample Controls" section — the only
controls table present documents the unrelated `TransformedCollisionTest` mouse/gamepad
test harness, which was not ported. The extractor correctly finds nothing rather than
picking up the unrelated table, but the result is an uninformative overlay. The other
two Collision Series tutorial samples (PerPixelCollision, RectangleCollision) have the
same gap for the same reason.
**Tracked in:** not planned (asset-generation limitation, not a CNA framework gap).
