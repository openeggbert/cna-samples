# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `TankOnAHeightmap.htm`) so the CNA-side blocker is documented in the same
place a future porting session will look. No `src/`/`CMakeLists.txt` exist yet.

Source: `/rv/tmp/XNAGameStudio/Samples/TankOnAHeightMapSample_4_0/TankOnAHeightmap/
TankOnAHeightmap/{TankOnAHeightmap.cs (266 lines), Tank.cs (262 lines),
HeightMapInfo.cs (189 lines)}`. (Note the source directory's unusual spelling —
`TankOnAHeightMapSample_4_0`, capital "M" — verified via `ls` against the actual
`/rv/tmp/XNAGameStudio/Samples/` listing.)

## Same blocker as `samples/SplitScreen/missing.md` — see that file for the full write-up
This sample's `Tank.cs` uses the **exact same tank rig** as `samples/SplitScreen`'s
`Tank.cs`: `md5sum` confirms `TankOnAHeightMapSample_4_0/.../Content/tank.fbx` and
`SplitScreenSample_4_0/.../SplitScreenSampleContent/tank.fbx` are byte-identical
(`467dbc460b52d7e3ea1de1df33991aa5`), and both `Tank.cs` files look up the identical
set of per-part bone names — `Bones["l_back_wheel_geo"]`, `r_back_wheel_geo`,
`l_front_wheel_geo`, `r_front_wheel_geo`, `l_steer_geo`, `r_steer_geo`, `turret_geo`,
`canon_geo`, `hatch_geo` — then animate them independently and combine them via
`CopyAbsoluteBoneTransformsTo`/per-mesh `ParentBone.Index` lookups exactly as
described in `samples/SplitScreen/missing.md`'s "Blocker: CNA's `.model.json` loader
has no per-mesh bone / bone hierarchy support" section. (The two `Tank.cs` files
differ in superficial ways — different namespace, some constants renamed/reordered,
and TankOnAHeightmap's version additionally takes a `HeightMapInfo` parameter in
`HandleInput` — but the bone-lookup/animation technique and the underlying model
asset are identical.)

**Do not re-derive this analysis here.** `samples/SplitScreen/missing.md` is the
canonical write-up: it already covers what CNA's `ModelTypeReader::Read()` and
`ModelMesh`/`Model::Draw()` are missing (only ever building one synthetic "Root"
`ModelBone`, no per-mesh bone assignment, `Model::Draw()` falling back to bone index
0 for every mesh), what would need to change in `cna` to fix it, and confirms the
existing `samples/CameraShake/Content/tank.model.json` asset already has the right
per-mesh names to work unmodified once the fix lands. All of that applies verbatim
to TankOnHeightmap too — read that file for the full technical detail.

**Tracked in:** DEFERRED.md item #6 (Model Asset Conversion / bone-hierarchy
pipeline gap) — same underlying gap as SplitScreen, ChaseCamera, SimpleAnimation,
CustomModelClassSample, and ModelViewerDemo (all six samples share this one
`tank.fbx` rig and technique).

## What's specific to TankOnHeightmap vs. SplitScreen
Beyond the shared tank/bone-hierarchy blocker, TankOnHeightmap adds a second, mostly
independent mechanic that SplitScreen does not have: **heightmap-based terrain
collision**, implemented in `HeightMapInfo.cs`/`TankOnAHeightmap.cs`:
- `TankOnAHeightmap.cs` loads a terrain `Model` whose `Tag` is a custom
  `HeightMapInfo` object (attached by a custom content pipeline processor,
  `TankOnAHeightmapPipeline/TerrainProcessor.cs` / `HeightMapInfoContent.cs`) rather
  than an ordinary flat/static model — the terrain's per-vertex height data and a
  precomputed normal grid are baked into the processed asset at build time.
- Each frame, `HeightMapInfo.IsOnHeightmap(cameraPosition)` and
  `GetHeightAndNormal(...)` are used to keep the camera (and the tank, via
  `tank.HandleInput(currentGamePadState, currentKeyboardState, heightMapInfo)`)
  clamped to the terrain surface and oriented to its local normal, instead of
  SplitScreen's flat ground plane.
- This terrain/heightmap piece is a **separate concern** from the tank bone-rig
  blocker above — it depends on CNA's static-model loading (already supported per
  DEFERRED.md item #6's "✅ CNA SUPPORTS STATIC MODELS" resolution) plus whatever
  custom processor logic bakes height/normal data into the terrain's `.model.json`
  or a side-car file. No CNA gap has been identified for the heightmap-collision
  math itself (it's plain per-triangle interpolation, not an engine feature); it is
  only blocked transitively because the sample as a whole can't run until the tank
  bone-hierarchy blocker is fixed.
