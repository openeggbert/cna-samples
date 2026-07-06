# Missing / Differences from XNA 4.0 original

**Status: UNBLOCKED, not yet ported — corrected 2026-07-06.** The blocker below was
accurate when first written this session, but a live build+run of `cna_test_
easygl_basiceffect_combinations` right after found CNA's `VertexPositionNormalTexture`
lit path (exactly what `Content.Load<Model>("terrain")`/`("sphere")` produces)
already works — case "(e) Directional lighting" passes (exit code 0). DEFERRED.md
item #5 is marked resolved for `Model`-based samples. No CNA gap remains; this is
now a normal, straightforward porting candidate. (Kept the original write-up below.)

Source: `/rv/tmp/XNAGameStudio/Samples/HeightmapCollisionSample_4_0/HeightmapCollision/HeightmapCollision/HeightmapCollision.cs`.

## Blocker: `BasicEffect.EnableDefaultLighting()` on the terrain and ball models

**XNA behaviour:** `DrawModel()` (`HeightmapCollision.cs:199-224`) loops over the
`terrain` (`Content.Load<Model>("terrain")`, `HeightmapCollision.cs:94`) and `sphere`
(`Content.Load<Model>("sphere")`, `HeightmapCollision.cs:108`) models' `mesh.Effects`
(all **stock** `BasicEffect` instances) and calls `effect.EnableDefaultLighting();` at
`HeightmapCollision.cs:214`, followed by `effect.PreferPerPixelLighting = true;` at
`HeightmapCollision.cs:215`, plus `FogEnabled`/`FogColor` setup. The sample's own
content pipeline addition (`HeightmapCollisionPipeline/TerrainProcessor.cs`,
`HeightMapInfoContent.cs`) only extracts heightmap sample data from the terrain's
bitmap for collision queries (`HeightMapInfo.cs`) — it defines no custom shader or
vertex format. Confirmed via direct source audit: **zero custom `.fx` files anywhere
in this sample** — all lighting is done through the stock `BasicEffect`, not a
hand-written shader. (Note: DEFERRED.md item #5 also flags that a sibling sample,
HeightmapCollision's cousin NormalMapping/BillboardSample, *does* ship a custom `.fx`
and is blocked by item #11 regardless — that does not apply here.)

**CNA port behaviour:** N/A yet (not ported). CNA's `BasicEffect` currently only
supports flat/unlit rendering with `VertexPositionColor` (as used by the Primitives3D
port); there is no `VertexPositionNormal` vertex struct and no per-vertex/per-pixel-lit
GLSL shader in the EasyGL backend, so neither `EnableDefaultLighting()` nor
`PreferPerPixelLighting` has anything to render with.

**Root cause (historical):** was a missing lit-shader path for `VertexPositionNormalTexture`
in CNA; now resolved (see Status note above).

**Tracked in:** DEFERRED.md item #5 (resolved)
