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
**CNA port behaviour:** No instruction text is drawn; no font asset was generated for
this sample and no `Content.Load<SpriteFont>`/`DrawString` call was ported.
**Root cause:** Port omission, not a CNA limitation — CNA's `SpriteFont` /
`SpriteBatch::DrawString` are fully implemented (used by several other samples, e.g.
InputReporter, CameraShake) but no `.font.json` atlas was ever generated for PathDrawing.
**Tracked in:** not planned (cosmetic instruction text only).

## Ground drawn by manual tiling instead of LinearWrap, without the "zoom" scaling
**XNA behaviour:** `SpriteBatch.Begin` with `SamplerState.LinearWrap` and a source
rectangle wider than the texture causes the GPU sampler to tile the texture; the source
rectangle is computed from `viewport size / groundSize` so each repeat unit is scaled to
occupy exactly `groundSize` (300) screen pixels regardless of the texture's native
resolution (the code comment calls this "zoom in/out on the ground").
**CNA port behaviour:** Ground texture (512×512 `ground.png`) is drawn at its native,
unscaled resolution in a loop stepping every `groundSize` (300) pixels — since 512 > 300,
each tile overlaps the previous one by 212px instead of tiling seamlessly at a true
300×300 cell size. The "zoom" scaling behaviour is lost entirely.
**Root cause:** CNA Vulkan backend does not yet forward the SamplerState from
`SpriteBatch::Begin` to the GPU sampler, so UV wrapping has no effect; the port's
workaround (manual grid of unscaled `Draw(texture, position, color)` calls) does not
reproduce the source-rectangle scaling the original relied on to make the tiles fit
`groundSize` exactly.
**Tracked in:** CNA issue (SamplerState forwarding); the scaling loss is an additional
port simplification, not planned separately.

## Draw order: path lines render on top of the tank instead of underneath it
**XNA behaviour:** `Draw()` renders back-to-front in this order: `DrawGround()` (its own
`spriteBatch.Begin()/End()` pass), then `DrawPath()` (the `primitiveBatch` line list), then
a second `spriteBatch.Begin()/End()` pass draws the instruction text and finally the tank
sprite — so the tank is always drawn last, on top of the path lines, hiding any line segment
that passes under it.
**CNA port behaviour:** `DrawGround()` and `tank_->Draw()` (plus the F1 help overlay) are
combined into a single `spriteBatch_->Begin()/End()` block, and `DrawPath()` is called
afterward, outside that block. This reverses the stacking order from the original: the path
line segments are now drawn on top of the tank sprite instead of underneath it.
**Root cause:** CNA has a known backend-specific bug (Vulkan) where a second `SpriteBatch`
`Begin()/End()` pair issued in the same frame discards the first render pass. The port
defensively merges what were two separate `SpriteBatch` passes in the original (ground pass,
then text/tank pass) into one `Begin()/End()` block to avoid this, which in turn forces
`DrawPath()` to run after the merged block instead of between the two original passes,
changing the tank/path draw order.
**Tracked in:** CNA issue (SpriteBatch multiple Begin/End per frame); the z-order change is
a side effect of the port's workaround, not planned separately.

## IsFullScreen and TargetElapsedTime omitted
**XNA behaviour:** Sets `IsFullScreen = true` and `TargetElapsedTime` to 30 fps (phone defaults).
**CNA port behaviour:** Default window mode and 60 fps.
**Root cause:** Phone-specific settings not applicable on desktop.
**Tracked in:** not planned.
