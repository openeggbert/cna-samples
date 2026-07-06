# Missing / Differences from XNA 4.0 original

**Status: UNBLOCKED, not yet ported — corrected 2026-07-06.** The blocker below was
accurate when first written this session, but a live build+run of `cna_test_
easygl_basiceffect_combinations` right after found CNA's `VertexPositionNormalTexture`
lit path (exactly what `Content.Load<Model>` produces here) already works — case
"(e) Directional lighting" passes (exit code 0). DEFERRED.md item #5 is marked
resolved for `Model`-based samples. No CNA gap remains; this is now a normal,
straightforward porting candidate. (Kept the original write-up below.)

Source: `/rv/tmp/XNAGameStudio/Samples/TrianglePickingSample_4_0/TrianglePickingSample/Game.cs`.

## Blocker: `BasicEffect.EnableDefaultLighting()` on the picked FBX models

**XNA behaviour:** `DrawModel()` (`Game.cs:588-603`) loops over every loaded model's
`mesh.Effects` (all **stock** `BasicEffect` instances, drawing the `table` and the
`ModelFilenames` FBX models via `Content.Load<Model>`, `Game.cs:175,186`) and calls
`effect.EnableDefaultLighting();` at `Game.cs:595`, followed by
`effect.PreferPerPixelLighting = true;` at `Game.cs:596`. The sample's own content
pipeline addition, `TrianglePickingProcessor` (a plain `ModelProcessor` subclass in
`TrianglePickingPipeline/TrianglePickingProcessor.cs`), only extracts per-triangle
vertex/index data for the ray-triangle intersection test — it defines no custom
shader or vertex format. Confirmed via direct source audit: **zero custom `.fx` files
anywhere in this sample** — all lighting is done through the stock `BasicEffect`, not
a hand-written shader.

**CNA port behaviour:** N/A yet (not ported). CNA's `BasicEffect` currently only
supports flat/unlit rendering with `VertexPositionColor` (as used by the Primitives3D
port); there is no `VertexPositionNormal` vertex struct and no per-vertex/per-pixel-lit
GLSL shader in the EasyGL backend, so neither `EnableDefaultLighting()` nor
`PreferPerPixelLighting` has anything to render with.

**Root cause (historical):** was a missing lit-shader path for `VertexPositionNormalTexture`
in CNA; now resolved (see Status note above).

**Tracked in:** DEFERRED.md item #5 (resolved)
