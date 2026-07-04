# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `SplitScreenSample.htm`) so the CNA-side blocker is documented in the same
place a future porting session will look. No `src/`/`CMakeLists.txt` exist yet —
see CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA gap
below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/SplitScreenSample_4_0/SplitScreenSample/
SplitScreenSample/{Program.cs (30 lines), SplitScreenGame.cs (225 lines), Tank.cs
(196 lines)}`. This is a *small* sample — the only reason it isn't already ported is
the one CNA gap below.

## Blocker: CNA's `.model.json` loader has no per-mesh bone / bone hierarchy support
**XNA behaviour:** `Tank.cs` (borrowed from the "Simple Animation" sample; this same
technique is also needed by SimpleAnimation, ChaseCamera, TankOnAHeightMap,
CustomModelClassSample, and ModelViewerDemo — all of which use the *exact same*
`tank.fbx`, confirmed byte-identical by `md5sum` against `samples/CameraShake/Content/
tank.model.json`'s source asset) animates the tank's wheels/steering/turret/cannon/
hatch independently by:
1. Looking up each part's bone by name: `tankModel.Bones["l_back_wheel_geo"]`,
   `Bones["r_back_wheel_geo"]`, `Bones["l_front_wheel_geo"]`, `Bones["r_front_wheel_geo"]`,
   `Bones["l_steer_geo"]`, `Bones["r_steer_geo"]`, `Bones["turret_geo"]`,
   `Bones["canon_geo"]`, `Bones["hatch_geo"]` (each an independent bone, not a nested
   skeleton — no skinning/blending, just per-part rigid transforms).
2. Caching each bone's original `Transform`, then each frame setting
   `bone.Transform = <rotation> * <originalTransform>` for the animated bones.
3. Calling `tankModel.CopyAbsoluteBoneTransformsTo(boneTransforms)` to combine every
   bone's local transform with its parent chain.
4. Drawing each `ModelMesh`, setting `effect.World = boneTransforms[mesh.ParentBone.Index]`
   per mesh (so each mesh renders using *its own* bone's absolute transform) before
   calling `mesh.Draw()`.

**CNA port behaviour:** N/A yet (not ported). Confirmed by reading
`cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`'s `ModelTypeReader::Read()`
(the `.model.json` reader) and `cna/src/.../Graphics/{Model,ModelMesh}.cpp`:
- The reader creates **exactly one `ModelBone`** total — a synthetic "Root" bone at
  index 0 (optionally named from a top-level `"bones"` JSON key, but no actual bone
  *array*/hierarchy is ever parsed from it) — see `ContentManager.cpp` lines ~449-473.
- Every mesh parsed from the `"meshes"` array (lines ~475-559) is added to the model's
  mesh list with **no parent bone ever assigned** — `ModelMesh` doesn't even expose a
  setter for it (`ModelMesh.hpp` declares `getParentBoneProperty()` but no
  `setParentBoneProperty`/bone-taking constructor overload; `parentBone_` stays
  `nullptr` for every `.model.json`-loaded mesh).
- `Model::Draw()` (`Model.cpp` ~line 100) falls back to bone index `0` (the Root) when
  a mesh's parent bone is null — so every mesh currently renders using the *same*
  single root transform. This is invisible for models that only need a static pose
  (Ground, CameraShake's tank — see its missing.md, it never animates per-part) but
  means `tankModel.Bones["l_back_wheel_geo"]` throws (no such bone exists) and there is
  no way to give one mesh an independent transform from another.
- `Model::CopyAbsoluteBoneTransformsTo` (`Model.cpp` ~line 35) itself is **already
  correct** for a real hierarchy — it walks each bone's parent chain via
  `bone->getParentProperty()`/`getIndexProperty()` assuming bones are stored
  parent-before-child. It just never receives more than one bone today.

**What's needed in `cna` (for whoever picks this up — not attempted here per user
request; this repo's `samples/SplitScreen/` currently has no source, only this
write-up):**
1. Add a way to give a `ModelMesh` a parent bone after construction (a setter, e.g.
   `setParentBoneProperty(ModelBone*)`, since `ModelTypeReader::Read()` builds meshes
   and bones in two separate passes) — `ModelMesh.hpp`/`.cpp`.
2. In `ModelTypeReader::Read()`, build one real `ModelBone` per mesh (parented to the
   existing Root bone, `bones_` ordered root-first so `CopyAbsoluteBoneTransformsTo`'s
   parent-chain walk stays correct) instead of only ever creating the Root. Default the
   new bone's name to the mesh's own `"name"` field unless an explicit `"bone"` field is
   added to the per-mesh JSON schema for cases where mesh name and bone name should
   differ.
3. **No content regeneration needed for this specific sample**: `samples/CameraShake/
   Content/tank.model.json` already has all 12 meshes named *exactly* what `Tank.cs`
   expects as bone names (`l_back_wheel_geo`, `r_back_wheel_geo`, `l_front_wheel_geo`,
   `r_front_wheel_geo`, `l_steer_geo`, `r_steer_geo`, `turret_geo`, `canon_geo`,
   `hatch_geo`, plus `tank_geo`/`r_engine_geo`/`l_engine_geo` which Tank.cs never
   animates) — a "one bone per mesh, named after the mesh" default makes this file work
   unmodified. The asset itself (`tank.model.json` + its `tank_*_verts.bin`/`_idx.bin`
   files) can just be copied from `samples/CameraShake/Content/` into
   `samples/SplitScreen/Content/` once the reader change lands — no new
   `assimp export`/`tools/obj2model.py` conversion pass is needed.

## Not blocked (confirmed while investigating)
- **Settable `GraphicsDevice.Viewport`** (`SplitScreenGame.cs` renders the scene twice
  through `playerOneViewport`/`playerTwoViewport`, restoring the old viewport after
  each): CNA has `GraphicsDevice::setViewportProperty(const Viewport&)` — already
  supported, no gap here.
- **`SpriteBatch`-drawn viewport divider lines**: ordinary 1x1-texture rectangle draws,
  same pattern used by several already-ported samples (e.g. DynamicMenu/UISample's
  blank-texture panels) — no gap here.
- The `assimp export *.fbx-or-*.x *.obj` → `tools/obj2model.py` → `.model.json`
  pipeline itself is proven twice now (CameraShake, PerformanceMeasuring) — the gap is
  specifically the *bone hierarchy*, not the mesh/vertex conversion.

**Tracked in:** DEFERRED.md (model/bone-hierarchy pipeline gap) — CNA-side fix to be
done directly in the `cna` repo (per user decision, not attempted in this repo).
