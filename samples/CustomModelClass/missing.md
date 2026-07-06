# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `CustomModel.htm`) so the CNA-side blocker is documented in the same place a
future porting session will look. No `src/`/`CMakeLists.txt` exist yet — see
CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA gap
below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/CustomModelClassSample_4_0/
{CustomModelSample/{CustomModel.cs, CustomModelSampleGame.cs}, CustomModelPipeline/
{CustomModelContent.cs, CustomModelProcessor.cs}}`.

## Blocker: BasicEffect per-vertex lighting (`EnableDefaultLighting`)
**XNA behaviour:** `CustomModel.cs:81` calls `effect.EnableDefaultLighting()` on the
`BasicEffect` used by every model part before drawing it (`CustomModel.Draw()`,
lines 72-106). The sample's whole point is demonstrating a simplified custom
replacement for XNA's `Model` class, but it still renders through a real,
lit `BasicEffect` — same class of gap as every other item #5 sample.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** CNA has no `VertexPositionNormal` vertex type or a normal-lit GLSL
shader in the backend yet, so `BasicEffect.EnableDefaultLighting()`/`LightingEnabled`
has nothing to render with for per-vertex-lit geometry (see DEFERRED.md item #5's
full text for the engine-level detail — `VertexPositionNormal.hpp` doesn't exist,
`VertexBuffer::SetData` isn't extended for it, and the EasyGL backend has no lit
shader for it).

**Note on why this is NOT also a bone-hierarchy (item #6) blocker, despite sharing
`tank.fbx` with SimpleAnimation/SplitScreen:** confirmed by grep — unlike `Tank.cs`
(used by SimpleAnimation/SplitScreen/ChaseCamera's sibling samples), this sample's
own `CustomModel.cs`/`CustomModelSampleGame.cs` contain **zero** `Bones[...]`
lookups. That's because this sample ships its **own** content-pipeline processor
(`CustomModelPipeline/CustomModelProcessor.cs`), which explicitly bakes every node's
transform into its geometry at content-build time and then discards the hierarchy
entirely (`ProcessNode()`: "Meshes can contain internal hierarchy ... but this sample
isn't going to bother storing any of that data" — `node.Transform = Matrix.Identity`
right after `MeshHelper.TransformScene`). The runtime `CustomModel` class is just a
flat `List<ModelPart>`, each drawn with the *same* shared `world` matrix
(`CustomModelSampleGame.cs`: `world = Matrix.CreateRotationY(time * 0.1f)`; the tank
rotates as one rigid whole, wheels/turret do not move independently in this sample).
So the model's per-mesh bone gap that blocks SimpleAnimation/SplitScreen simply
doesn't apply here — DEFERRED.md item #6's summary table also lists this sample
under the bone-hierarchy blocked set, but that appears to be inherited from "uses the
shared `tank.fbx` asset" rather than a per-sample source audit; item #5's own section
(which does list this sample by name against the actual `EnableDefaultLighting()`
call site) is the accurate one for this specific sample, confirmed directly against
the C# source above.

**Tracked in:** DEFERRED.md item #5.
