# Missing / Differences from XNA 4.0 original

## Touch input replaced with mouse input
**XNA behaviour:** The sample targets Windows Phone and uses `TouchPanel` / `TouchCollection`
for touch-to-draw interaction: tap on tank to start path, drag to extend it.
**CNA port behaviour:** Mouse input via `Mouse::GetState()` replaces touch. Left-click on
the tank starts a new path; hold and drag to draw the path.
**Root cause:** CNA targets desktop (no touch panel abstraction). Mouse is the natural substitute.
**Tracked in:** not planned (desktop-only target).

## SpriteFont / DrawString omitted
**XNA behaviour:** Displays "Drag a path from the tank to have him drive around." using
a `SpriteFont` loaded from `Font.spritefont`.
**CNA port behaviour:** No instruction text is drawn; SpriteFont is not yet implemented in CNA.
**Root cause:** CNA has no SpriteFont support.
**Tracked in:** DEFERRED.md.

## Ground drawn by manual tiling instead of LinearWrap
**XNA behaviour:** `SpriteBatch.Begin` with `SamplerState.LinearWrap` and a source
rectangle wider than the texture causes the GPU sampler to tile the texture.
**CNA port behaviour:** Ground texture drawn in a loop of `groundSize × groundSize`
pixel tiles using the default SpriteBatch (no LinearWrap needed).
**Root cause:** CNA Vulkan backend does not yet forward the SamplerState from
`SpriteBatch::Begin` to the GPU sampler, so UV wrapping has no effect.
**Tracked in:** CNA issue.

## IsFullScreen and TargetElapsedTime omitted
**XNA behaviour:** Sets `IsFullScreen = true` and `TargetElapsedTime` to 30 fps (phone defaults).
**CNA port behaviour:** Default window mode and 60 fps.
**Root cause:** Phone-specific settings not applicable on desktop.
**Tracked in:** not planned.
