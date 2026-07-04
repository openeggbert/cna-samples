# Missing / Differences from XNA 4.0 original

## DualTextureEffect: single shared UV instead of two independent UV channels
**XNA behaviour:** `PlanePrimitiveDualTextured` uses a custom vertex format
(`VertexPositionNormalDualTexture`, with `TextureCoordinate0`/`TextureCoordinate1`)
so the base texture ("Base") and detail texture ("Detail") tile at *different* rates
(`Tiling1` = 10×10 for the base, `Tiling2` = 30×30 for the detail texture) — the whole
point of the sample is this independently-tiled detail-mapping effect.
**CNA port behaviour:** CNA's `DualTextureEffect` is fully implemented
(`FillGpuDrawParams`, a real EasyGL shader), but that shader only has **one** shared
UV input for both textures:
```glsl
FragColor = texture(uTexture, vUV) * texture(uTexture2, vUV) * uDiffuseColor;
```
There is no second UV attribute anywhere in the EasyGL backend's dual-textured
program. This port uses the built-in `VertexPositionNormalTexture` (single UV) with
one shared tiling factor for `PlanePrimitiveDualTextured` instead of the original's
two-UV-channel vertex format — both textures now tile at the same rate. The detail
texture still visibly blends with the base (confirmed by screenshot — the pitch shows
a mottled/darker detail pattern over the green base), just without the original's
independent tiling scales. User-confirmed decision: port with this documented visual
simplification rather than block on a CNA shader change.
**Root cause:** `EnsureDualTextured3DProgram()` in
`cna/src/CNA/Internal/Backends/EasyGL/EasyGLGraphicsBackend.cpp` only declares one
`in vec2 aUV`.
**Tracked in:** DEFERRED.md (would need a second UV attribute + a
`VertexPositionNormalDualTexture`-equivalent type/generic vertex declaration support
in the EasyGL backend — not attempted here, no CNA changes made).

## Unported: `PlanePrimitive` (untextured helper, unreferenced)
**XNA behaviour:** `PlanePrimitive.cs` (`VertexPositionNormal`, no UVs) exists in the
original project.
**CNA port behaviour:** Not ported — `SoccerPitchGame.cs` never references
`PlanePrimitive` anywhere; it's dead code in the original (likely left over from a
shared template project). No behavior lost.
**Tracked in:** not planned.

## Touch input supplemented with mouse fallback
**XNA behaviour:** `TouchPanel.GetState()` is polled every `Update`; a `Released`
touch toggles Alpha-Test/Alpha-Blend.
**CNA port behaviour:** The same `TouchPanel` polling is kept as-is (harmless no-op on
a desktop with no touchscreen), plus a left-click rising-edge fallback that toggles
the same state, matching this project's established touch/mouse-fallback pattern.
**Root cause:** This desktop has no touchscreen and CNA does not synthesize touch
from mouse input.
**Tracked in:** not planned — established project pattern (see NEXT.md section 6).

## FrameRateCounter: explicit RootDirectory + GraphicsDevice wiring
**XNA behaviour:** `FrameRateCounter` constructs its own `ContentManager(game.Services)`
with a blank `RootDirectory`, then loads `"content\\Font"` (a literal path prefix
compensating for the blank root).
**CNA port behaviour:** The port's `ContentManager` has `RootDirectory` explicitly set
to `"Content"` and loads plain `"Font"` — same asset, tidier path, identical result.
Also, unlike XNA (where the framework wires every `ContentManager` to the graphics
device automatically), CNA's `ContentManager` needs an explicit
`setGraphicsDevice(getGraphicsDeviceProperty())` call before its first `Load<T>()` for
a GPU resource — the original never needed this since XNA's `Game.Services`-based
`ContentManager` construction already had the device wired; CNA's manually-constructed
second `ContentManager` does not without this call. Found via a real startup crash
(`ContentLoadException: GraphicsDevice is not set`) during interactive verification.
**Tracked in:** not planned — call-site fix, not a CNA gap (the game's own
`Content` property is wired automatically by the framework; only a manually
constructed second `ContentManager`, as this component uses, needs the explicit call).

## Interactive verification
Confirmed by screenshot: the pitch (dual-textured, single shared UV per above) with
the center-circle/halfway-line stripe overlay, the soccer ball with its shadow, the
camera fly-over animation (lerping between a wide establishing shot and a close-up on
the ball), the FPS counter, and both the Alpha-Blend (smooth, additive-blended lines)
and Alpha-Test (crisp, non-blended lines) rendering paths — confirmed by toggling with
a mouse click and observing the on-screen mode label and line-rendering style change.
The F1 help overlay (hand-authored text, since `SoccerPitchOverview.htm` — the only
`.htm`-like doc file this kit shipped, named differently from the `SampleName.htm`
convention — has no "Sample Controls" table) was also confirmed.
