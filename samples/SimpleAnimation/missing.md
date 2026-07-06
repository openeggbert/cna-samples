# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `SimpleAnimation.htm`) so the CNA-side blocker is documented in the same
place a future porting session will look. No `src/`/`CMakeLists.txt` exist yet —
see CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA gap
below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/SimpleAnimation_4_0/SimpleAnimation/
{SimpleAnimation.cs, Tank.cs}`. This sample **is `Tank.cs`** — the whole point of the
sample is animating the tank's wheels/steering/turret/cannon/hatch independently.

## Blocker: CNA's `.model.json` loader has no per-mesh bone / bone hierarchy support
**XNA behaviour:** `Tank.cs` (139-146: `tankModel.Bones["l_back_wheel_geo"]`,
`Bones["r_back_wheel_geo"]`, `Bones["l_front_wheel_geo"]`, `Bones["r_front_wheel_geo"]`,
`Bones["l_steer_geo"]`, `Bones["r_steer_geo"]`, `Bones["turret_geo"]`,
`Bones["canon_geo"]`, `Bones["hatch_geo"]`) looks up each part's bone by name, caches
its original `Transform`, rewrites `bone.Transform = <rotation> * <originalTransform>`
per animated bone every frame, calls `tankModel.CopyAbsoluteBoneTransformsTo(...)`,
and draws each `ModelMesh` with `effect.World = boneTransforms[mesh.ParentBone.Index]`.

**CNA port behaviour:** N/A yet (not ported).

**Root cause / canonical write-up:** This is the **exact same** blocker, the exact
same technique, and the exact same `tank.fbx` source asset (this sample is in fact
where `Tank.cs` originates — `SplitScreen`'s `Tank.cs` and this sample's `Tank.cs` are
the same file) already fully documented in `samples/SplitScreen/missing.md`. That
write-up cites the precise `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`
(`ModelTypeReader::Read()`) and `cna/src/.../Graphics/{Model,ModelMesh}.cpp` evidence:
CNA's `.model.json` reader only ever constructs one synthetic "Root" `ModelBone` for
the whole model, every mesh's parent bone stays null (no setter even exists for it),
and `Model::Draw()` falls back to bone index 0 for every mesh — so no mesh can ever
get an independent transform from any other. See `samples/SplitScreen/missing.md` for
the full analysis (exact file/line evidence, and the 3-step "what's needed in `cna`"
plan) rather than repeating it here; nothing about the gap differs for this sample.
As that write-up notes, `samples/CameraShake/Content/tank.model.json` already has
mesh names matching every bone name `Tank.cs` expects, so once the CNA reader change
lands, the asset can likely be reused/copied with no new conversion pass for this
sample either.

**Tracked in:** DEFERRED.md item #6 ("Model Asset Conversion (FBX/X → .model.json)"),
specifically the "rigid multi-part bone hierarchy" gap documented inside that item's
section (which explicitly lists SimpleAnimation among the samples blocked on it,
alongside SplitScreen, TankOnAHeightMap, CustomModelClassSample, and ModelViewerDemo).
