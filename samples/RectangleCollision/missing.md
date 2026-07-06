# Missing / Differences from XNA 4.0 original

## Color key not applied when converting sprite assets
**XNA behaviour:** `Block.bmp` and `Person.bmp` are compiled through the default
`TextureImporter`/`TextureProcessor` (no overrides in
`RectangleCollisionContent.contentproj`), which has `ColorKeyEnabled = true` and
`ColorKeyColor = (255,0,255,255)` (magenta) by default. The magenta background of
both source bitmaps is therefore converted to fully transparent pixels at build
time, so the block and person sprites render as a black triangle / black-and-white
stick figure with a transparent background over the cornflower-blue play field
(see `Documentation/FallingBlocks.png`).
**CNA port behaviour:** `Content/Block.png` and `Content/Person.png` were converted
directly from the BMP files with no color-keying applied — the magenta background
pixels remain fully opaque (`(255,0,255,255)`). The sprites render as solid magenta
squares with the shape drawn inside them, instead of a transparent-background
shape.
**Root cause:** Porting oversight — CNA does not run the XNA Content Pipeline, so
color-keying must be applied manually when converting art (as was done correctly
for e.g. the TransformedCollision and PerPixelCollision samples' assets), and it
was not done here.
**Tracked in:** not planned (asset fix, no CNA framework change needed).

## Color named constants replaced with RGBA literals
**XNA behaviour:** Uses `Color.Red`, `Color.CornflowerBlue`, and `Color.White`
named constants.
**CNA port behaviour:** Equivalent RGBA values used directly:
`Color(255,0,0,255)`, `Color(100,149,237,255)`, `Color(255,255,255,255)`.
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
sample's `.htm` documentation, but `RectangleCollision.htm` is a step-by-step
tutorial page (Collision Series 1) that has no "Sample Controls" section, so the
extractor correctly finds nothing rather than fabricating one. The other two
Collision Series tutorial samples (PerPixelCollision, TransformedCollision) have the
same gap for the same reason.
**Tracked in:** not planned (asset-generation limitation, not a CNA framework gap).
