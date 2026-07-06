# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `HeightmapCollision.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

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

**Root cause:** Missing `VertexPositionNormal` struct + normal-lit shader in CNA
(DEFERRED.md item #5), not a missing asset-conversion pipeline.

**Tracked in:** DEFERRED.md item #5
