# Missing / Differences from XNA 4.0 original

**Status: ported (2026-07-11).** Source: `/rv/tmp/XNAGameStudio/Samples/
SimpleAnimation_4_0/SimpleAnimation/{SimpleAnimation.cs, Tank.cs}`. This sample
**is `Tank.cs`** — the whole point of the sample is animating the tank's
wheels/steering/turret/cannon/hatch independently; `SimpleAnimation.cs` itself
is a ~30-line wrapper (constructs a `Tank`, drives 5 animation properties from
`sin`/time each frame, sets up a fixed orbiting camera, no other content —
not even a `SpriteFont`/HUD text of any kind in the real original).

This was the first of 3 samples (SimpleAnimation, SplitScreen, TankOnHeightmap)
sharing the same blocker and the same `tank.fbx` asset, picked specifically to
validate the newly-landed `cna` multi-bone fix (Tasks 936/937, see below) in
isolation before the other two build on top of it.

## Multi-bone `ModelBone` gap — now fixed in `cna`, but with a real remaining gap this port had to work around

**XNA behaviour:** `Tank.cs` looks up 9 bones by name (`tankModel.Bones["l_back_wheel_geo"]`,
`r_back_wheel_geo`, `l_front_wheel_geo`, `r_front_wheel_geo`, `l_steer_geo`,
`r_steer_geo`, `turret_geo`, `canon_geo`, `hatch_geo`), caches each one's
*original* `Transform` (the part's correct rest-pose offset from its own real
parent bone, as built by XNA's own content pipeline from `tank.fbx`'s real,
nested node hierarchy), then each frame sets `bone.Transform = <animRotation> *
<originalTransform>`, calls `tankModel.CopyAbsoluteBoneTransformsTo(...)`, and
draws each `ModelMesh` with `effect.World = boneTransforms[mesh.ParentBone.Index]`.

**What changed in `cna` since this was last investigated (confirmed 2026-07-11,
by direct source read of the current `ContentManager.cpp`, not assumed from
commit messages):** Tasks 936/937 (landed 2026-07-10) added
`ModelMesh::setParentBoneProperty(ModelBone*)` and changed `ModelTypeReader::Read()`
to build one real `ModelBone` per mesh (a named child of the model's synthetic
Root) instead of leaving every mesh's `ParentBone` null. This is exactly the
2-step fix this sample's (and `SplitScreen`'s) own `missing.md` previously
asked for — `Model.Bones["l_back_wheel_geo"]` etc. now resolve to a real,
distinct bone per part, and `CopyAbsoluteBoneTransformsTo` produces a distinct
absolute transform per mesh. **This alone was enough to unblock writing this
port** — no `cna` edit was needed to ship it.

**Real remaining gap found while actually porting (not just the pre-existing
"transform defaults to Identity" caveat DEFERRED.md item #6 already flagged —
the exact mechanism needed to be nailed down empirically):**

1. **Confirmed `tank.fbx`'s own node hierarchy is genuinely nested, not flat**
   (read directly, `Connect: "OO"` lines): `tank_geo` (root/body) has 3
   direct children — `r_engine_geo`, `l_engine_geo`, `turret_geo` — each of
   which has further children (`r_engine_geo` → `r_back_wheel_geo`,
   `r_steer_geo` → `r_front_wheel_geo`; mirrored on the left; `turret_geo` →
   `canon_geo`, `hatch_geo`). `l_engine_geo`/`r_engine_geo` are themselves
   real meshes (not just pivot nulls) — CameraShake's already-shipped
   `tank.model.json` (reused byte-identically for this port, see below) has
   `tank_l_engine_geo_verts.bin`/`tank_r_engine_geo_verts.bin` files, so `cna`'s
   flat, one-mesh-one-bone-under-Root reader gives them their own bone too,
   with the same Identity-transform gap as the 9 animated parts.
2. **Confirmed every node's own `Lcl Translation` is non-zero and every node's
   own `Lcl Rotation`/`PreRotation` is exactly zero** (grepped all 12 relevant
   `Model::` nodes in `tank.fbx` directly). Since `cna`'s reader parents every
   mesh directly to Root with an Identity `Transform` (no nesting, no
   per-mesh rest-transform field in the `.model.json` schema at all — this
   part of DEFERRED.md item #6 is still open), every part would render
   stacked at the tank body's own local mesh origin without further work —
   confirmed this is a real, not just a hypothetical, gap: `tank.model.json`'s
   own per-part vertex data (`tank_l_back_wheel_geo_verts.bin` etc.) is
   genuinely local to each part (e.g. `l_back_wheel_geo`'s raw vertex range is
   centered near its own local pivot, not translated into tank-body space —
   confirmed by direct comparison against `tank.fbx`'s own raw `Vertices:`
   block, byte-for-byte identical to the shipped `_verts.bin`, i.e. this
   asset was converted by the *pre-PreRotation/LclTranslation-baking* version
   of `tools/fbx_ascii2model.py`, so nothing FBX-node-level is baked into any
   mesh's vertices at all).

**Port-side fix used (NOXNA, no `cna` edit):** `Tank::Load()`'s new
`ApplyRestTransforms()` (see `src/Tank.hpp`) sets each of the 11 non-root
meshes' `ModelBone::Transform` directly, via `Matrix::CreateTranslation(x, y, z)`,
to its correct **absolute** rest offset from the tank body — i.e. the real
XNA content pipeline's own per-node `Transform` *composed through the real,
nested parent chain* (e.g. `l_back_wheel_geo`'s value folds in its own `Lcl
Translation` **and** its real parent `l_engine_geo`'s `Lcl Translation`, since
`cna`'s flat bone tree has no `l_engine_geo` bone standing between them).
Since every rotation in this asset is exactly zero, composing through the
chain is a plain vector sum, not a general matrix multiply — computed once via
a short Python script that parses `tank.fbx`'s own `Model::` node properties
and `Connect: "OO"` hierarchy directly (not committed to `tools/`, values
hardcoded into `Tank.hpp` as a small lookup table) and cross-checked against
hand computation. This exactly reproduces what `CopyAbsoluteBoneTransformsTo`
would already do correctly on a properly-nested bone tree — it is
mathematically identical to real XNA's own result, just computed with the
composition folded in ahead of time instead of at bone-hierarchy-walk time.
Everything else in `Tank::Draw()` (the 9 bones' own per-frame
`bone.Transform = animRotation * originalTransform`, `CopyAbsoluteBoneTransformsTo`,
the per-mesh `effect.World = boneTransforms[mesh->getParentBoneProperty()->getIndexProperty()]`
loop) is an unmodified, direct port of `Tank.cs`'s own technique using the
real `Model`/`ModelBone`/`ModelMesh` API — no NOXNA bypass class was needed,
unlike `ReachGraphicsDemo`'s own `TankModel.hpp` (a different, unrelated
12-part tank asset from the MIX10 demo, written before this `cna` fix
existed — consulted only as a reference for "does a real nested rig need this
kind of treatment," not reused directly, since its own hand-rolled
parent-index/chain-multiply approach is unnecessary now that
`ModelBone`/`CopyAbsoluteBoneTransformsTo` can be used directly).

**Confirmed live, screenshot-verified:** all 4 wheels, both steer pivots, the
turret, cannon, and hatch all render in correct relative position (wheels at
the 4 corners, turret centered on top with the cannon extending forward, hatch
on the turret roof) across multiple frames with the camera orbiting and every
part's own animation (wheel spin, steering sweep, turret swivel, cannon
elevation, hatch open/close) visibly active and structurally coherent frame to
frame — see this session's own screenshots (not committed to the repo).
Without `ApplyRestTransforms()` (tested during development), every part
rendered piled at the tank body's own local origin, confirming the gap
description above is exactly right, not just theoretically necessary.

**Tracked in:** DEFERRED.md item #6 (multi-bone addendum — mostly resolved,
see its own updated write-up; the specific "no per-mesh rest-transform field
in `.model.json`" sub-gap this port worked around remains open, since fixing
it in `cna` would need a schema change, not just the bone-per-mesh change that
already landed).

## Reused, unmodified `tank.model.json` — no asset regeneration needed
`samples/CameraShake/Content/tank.model.json` (plus its 12 meshes' `_verts.bin`/
`_idx.bin` pairs) was copied byte-for-byte into this sample's own `Content/`
directory, with **no reconversion**. This confirms DEFERRED.md item #6's own
speculation that the existing asset could likely be reused once the reader fix
landed, since its mesh names already match every bone name `Tank.cs` expects.

## Flat, untextured shading (same known, already-documented characteristic — not new)
**XNA behaviour:** every tank part renders with its own real diffuse material
texture (`steamroller_tank61_file3`/`file1`, per `tank.fbx`'s own `Material:`/
`Texture:` blocks).
**CNA port behaviour:** every part renders flat white/cream with only shading
gradient, no texture detail.
**Root cause:** `tank.model.json` predates `cna` Task 932 (per-mesh `"texture"`
field support in `ModelTypeReader`) and has no `"texture"` entry on any mesh —
the same already-documented DEFERRED.md item #6 "flat white saturation"
finding first confirmed on PickingSample and reconfirmed live on CameraShake
this same session (see NEXT.md's 2026-07-11 entry). Not re-investigated here;
regenerating `tank.model.json` with real per-mesh textures (`Tank.png`/
`Turret.png`, already converted and shipped in `samples/CameraShake/Content/`)
is a real, low-effort follow-up someone could do, but out of scope for this
task (which was specifically about validating the multi-bone fix).
**Tracked in:** DEFERRED.md item #6.

## No background/ground plane, no HUD text (faithful to the original)
**XNA behaviour:** `SimpleAnimation.cs`'s `Draw()` only clears to `Color.DarkGray`
and draws the tank — no ground plane, no `SpriteFont`/instructions text of any
kind (confirmed by a full read of the 4.5 KB source file — there is no
`SpriteBatch`/`SpriteFont` reference anywhere in it).
**CNA port behaviour:** identical — `Content/` ships no font asset at all
(deliberately not copied from CameraShake, since the real original never uses
one), matching the original's own minimalism exactly.

## Explicit backbuffer size set (NOXNA, real original has none)
**XNA behaviour:** `SimpleAnimationGame`'s constructor sets no
`PreferredBackBufferWidth`/`Height` at all, so real XNA falls back to its own
800×480 default.
**CNA port behaviour:** explicitly sets `1280×720` in the constructor.
**Root cause:** confirmed live this session — with no explicit size set, CNA's
own `GraphicsDeviceManager` still reports an 800×480 `Viewport` internally
(matching XNA's default), but the actual SDL window this machine's display
server created was a much larger `1740×1044`, a ~2.175× logical/physical
mismatch (this machine's own display scaling, not reproduced/investigated
further as a `cna` bug — no DEFERRED.md item filed, since every other 3D
sample in this repo already explicitly sets its own backbuffer size and was
therefore never exposed to this default-size path). Setting an explicit size
sidesteps it entirely and matches this repo's own established convention
(CameraShake and most other desktop-targeted 3D samples already do this even
though their own originals don't either). **Symptom this caused, worth noting
for future porters relying on the implicit default:** the F1 help overlay's
own centering math (`(vp.Width - hw) / 2` etc.) is computed correctly against
the *logical* 800×480 viewport, but the actual on-screen sprite ends up
positioned according to that logical space scaled into the larger physical
window — visually it still centers correctly (confirmed by the arithmetic,
not just luck), but pixel-space debugging (e.g. `import -window` screenshot
tooling sampling a specific pixel) must account for the scale factor, not
assume 1:1 logical-to-physical pixels, if this default-size path is ever hit
again.

## Verification
Builds 0 warnings (both a targeted rebuild and a full aggregate rebuild from
the previous state, `ninja: no work to do` on immediate re-run). Ran 5+ seconds
with no crash across 3 separate runs. Confirmed live via screenshot: the tank
renders fully assembled and correctly proportioned from multiple camera
angles (the world rotates over time per the original's own
`Matrix.CreateRotationY(time * 0.1f)`); wheel/steer/turret/cannon/hatch
animation all visibly active and structurally coherent across frames; F1 help
overlay renders correctly centered with the expected control text and
disappears after its 10-second timer (confirmed via this repo's established
temporary debug-auto-trigger pattern — forced `helpTimer_ = 10.0f` on load,
reverted before commit — since a different real user window held focus
throughout, confirmed via `xdotool getactivewindow`, so no synthetic keypress
was sent). No new DEFERRED.md item filed for the core blocker (item #6 already
covers it, updated status only); no item filed for the backbuffer-size finding
above either, since it's a documented per-sample convention choice, not a
confirmed `cna` bug.
