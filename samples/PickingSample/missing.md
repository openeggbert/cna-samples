# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `Picking.htm`) so the CNA-side blocker is documented where a future porting
session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's "Adding a
new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/PickingSample_4_0/Picking/Game.cs`.

## Blocker: `BasicEffect.EnableDefaultLighting()` on all picked FBX models

**XNA behaviour:** `DrawModel()` (`Game.cs:294-311`) loops over every loaded model's
`mesh.Effects` (all **stock** `BasicEffect` instances, drawing the `table`, `sphere`,
`cylinder`, `cats`, and `p2wedge` FBX models listed via `ModelFilenames` /
`Content.Load<Model>`, `Game.cs:131,142`) and calls `effect.EnableDefaultLighting();`
at `Game.cs:304` before rendering. Confirmed via direct source audit: **zero custom
`.fx` files anywhere in this sample** — every model is lit purely through the stock
`BasicEffect`, not a hand-written shader.

The project also carries a `GeometricPrimitive.cs` file (from the same Primitives3D
lineage referenced in DEFERRED.md item #5) that defines its **own** `VertexPositionNormal`
vertex struct (`List<VertexPositionNormal> vertices`, `GeometricPrimitive.cs:33`;
`vertexBuffer = new VertexBuffer(graphicsDevice, typeof(VertexPositionNormal), ...)`,
`GeometricPrimitive.cs:93`) and calls `basicEffect.EnableDefaultLighting();` at
`GeometricPrimitive.cs:107`. Note: this file is **not** included in
`Picking (Windows).csproj`'s `<Compile>` list (dead/unused code left over from a
copy-paste), so it is not the sample's actual runtime lighting path — `Game.cs:304`
is. It is mentioned here only because DEFERRED.md's audit cites it as evidence that
this sample family expects a `VertexPositionNormal`-shaped vertex.

**CNA port behaviour:** N/A yet (not ported). CNA's `BasicEffect` currently only
supports flat/unlit rendering with `VertexPositionColor` (as used by the Primitives3D
port); there is no `VertexPositionNormal` vertex struct and no lit GLSL shader in the
EasyGL backend, so `EnableDefaultLighting()` has nothing to render with.

**Root cause:** Missing `VertexPositionNormal` struct + normal-lit shader in CNA
(DEFERRED.md item #5), not a missing asset-conversion pipeline.

**Tracked in:** DEFERRED.md item #5
