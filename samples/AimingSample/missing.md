# Missing / Differences from XNA 4.0 original

## Spotlight additive blending omitted
**XNA behaviour:** The cat is drawn in its own `spriteBatch.Begin()`/`End()` pair, then the spotlight is drawn in a second, separate `spriteBatch.Begin(SpriteSortMode.FrontToBack, BlendState.Additive)`/`End()` pair, giving it a glowing light effect over the black background.
**CNA port behaviour:** The spotlight is drawn with default alpha blend in the same `SpriteBatch::Begin()/End()` block as the cat (and the F1 help overlay). The spotlight still rotates and aims correctly, but has no additive glow.
**Root cause:** CNA's Vulkan backend has a bug where only the last `SpriteBatch::Begin()/End()` pair per frame is rendered, so porting the original's two-Begin/End structure verbatim would make the cat invisible on that backend. Merging both draws into one block with default blend sidesteps this unconditionally. Note this is stricter than the project's current default backend actually requires: `CMakeLists.txt` now defaults `CNA_GRAPHICS_BACKEND` to `EASYGL`, and multiple Begin/End pairs per frame do compose correctly there (confirmed by GameStateManagement #072's `missing.md`, which keeps its screens in separate Begin/End pairs and only calls out Vulkan as broken). A faithful two-Begin/End port with `BlendState.Additive` would render correctly on EasyGL — only Vulkan would break — but the single-block workaround was kept here for Vulkan-backend safety.
**Tracked in:** CNA issue (Vulkan multi-batch bug)

## Windows Phone portrait/fullscreen branch removed
**XNA behaviour:** The constructor sets `graphics.SupportedOrientations = DisplayOrientation.Portrait` unconditionally, then `#if WINDOWS_PHONE` selects a 480x800 full-screen back buffer with a fixed 30 fps `TargetElapsedTime`; `#else` (Windows/Xbox) selects an 853x480 windowed back buffer, which is the branch this port follows.
**CNA port behaviour:** Only the non-phone 853x480 windowed branch is ported (default frame timing); no phone-orientation/full-screen/fixed-timestep path exists, and `SupportedOrientations` is never set.
**Root cause:** Desktop-only port target; Windows Phone-specific platform code is out of scope.
**Tracked in:** not planned (intentional desktop adaptation).

## Viewport X/Y offset ignored
**XNA behaviour:** `Initialize()` reads `vp.X` and `vp.Y` to offset spotlight and cat positions, supporting non-zero viewport origins.
**CNA port behaviour:** Viewport X/Y are assumed to be 0 and omitted from the position calc. CNA's `Viewport` class does have public `x`/`y` int fields (unlike `Width`/`Height`, they are plain members, not wrapped in a `getXProperty()`/`setXProperty()` pair), but `GraphicsDevice::UpdateViewportFromWindow()` always hardcodes `viewport_.x = 0; viewport_.y = 0;`, so reading them would be a no-op for every CNA backend today. In practice this is correct for the full-screen viewports this sample uses.
**Root cause:** CNA's viewport is always window-anchored at (0,0); no backend currently produces a sub-viewport with a non-zero origin.
**Tracked in:** CNA issue (would need a real use case for offset viewports to prioritize)

## Texture converted from TGA to PNG
**XNA behaviour:** Loads `cat.tga` (a 128x128 RLE-compressed Targa file, see `AimingContent/cat.tga`) via `Content.Load<Texture2D>("cat")`. `spotlight.png` was already a PNG in the original content project and needed no conversion.
**CNA port behaviour:** `cat.tga` was converted to `Content/cat.png`, loaded via the same `Content.Load<Texture2D>("cat")` call (CNA's `ContentManager` resolves the PNG file for that key).
**Root cause:** CNA's `ContentManager`/asset pipeline does not support `.xnb`/TGA source assets (see CLAUDE.md Assets section); source art was converted to PNG, matching the same TGA→PNG conversion documented for Audio3D's `CatTexture.tga` and GesturesSample's `cat.tga`.
**Tracked in:** Not planned — standard asset-conversion step for this project.
