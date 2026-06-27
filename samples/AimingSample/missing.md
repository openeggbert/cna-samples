# Missing / Differences from XNA 4.0 original

## Spotlight additive blending omitted
**XNA behaviour:** The spotlight texture is drawn with `BlendState.Additive`, giving it a glowing light effect over the black background.
**CNA port behaviour:** The spotlight is drawn with default alpha blend in the same `SpriteBatch::Begin()/End()` block as the cat. The spotlight still rotates and aims correctly, but has no additive glow.
**Root cause:** CNA has a Vulkan backend bug where only the last `SpriteBatch::Begin()/End()` pair per frame is rendered. Using two separate Begin/End pairs (one per blend mode) would make the cat invisible. Merging both into one block with default blend is the pragmatic workaround.
**Tracked in:** CNA issue (Vulkan multi-batch bug)

## Viewport X/Y offset ignored
**XNA behaviour:** `Initialize()` reads `vp.X` and `vp.Y` to offset spotlight and cat positions, supporting non-zero viewport origins.
**CNA port behaviour:** Viewport X/Y are assumed to be 0 (CNA Viewport has no `getXProperty()`/`getYProperty()`). In practice this is correct for full-screen viewports.
**Root cause:** CNA Viewport does not expose X/Y properties.
**Tracked in:** CNA issue
