# Missing / Differences from XNA 4.0 original

**Status: UNBLOCKED, not yet ported — corrected 2026-07-06.** The blocker below was
accurate when first written this session, but a live build+run of `cna_test_
easygl_basiceffect_combinations` right after found CNA's `VertexPositionNormalTexture`
lit path already works — case "(e) Directional lighting" passes (exit code 0).
DEFERRED.md item #5 is marked resolved for `Model`-based samples. The port should
just use the standard `tools/obj2model.py` conversion + stock `Model`/`BasicEffect`
(CNA has no generic custom-`ContentTypeReader` extensibility to replicate this
sample's own `CustomModel`/`CustomModelProcessor` faithfully anyway — see item #18)
rather than trying to reproduce the C# original's custom content type. No CNA gap
remains; this is now a normal porting candidate. (Kept the original write-up below.)

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

**Root cause (historical):** was a missing lit-shader path for `VertexPositionNormalTexture`
in CNA; now resolved (see Status note above).

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

**Tracked in:** DEFERRED.md item #5 (resolved).
