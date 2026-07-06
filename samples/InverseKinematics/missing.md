# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `InverseKinematics.htm`) so the CNA-side blocker is documented in the same
place a future porting session will look. No `src/`/`CMakeLists.txt` exist yet — see
CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA gap
below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/InverseKinematics_4_0/InverseKinematics/
InverseKinematics/{IKSample.cs, Cat.cs}` plus
`InverseKinematicsContent/{cylinder.x, cat.tga, font.spritefont}`. The sample shows
the Cyclic Coordinate Descent (CCD) IK algorithm two ways: a 20-link chain of lit
cylinder models reaching for the mouse/`cat`, and the same CCD chain driving an
Xbox LIVE avatar's arm/head to reach for (and look at) the cat.

## Blocker: BasicEffect per-vertex lighting (`EnableDefaultLighting`)
**XNA behaviour:** `IKSample.cs`'s `LoadCylinderModel()` (around line 210) loads
`cylinder.x` and, for every mesh effect, calls
`basicEffect.EnableDefaultLighting()` plus `basicEffect.PreferPerPixelLighting = true`
(line 226) — the cylinder chain that visualizes the IK solution is lit `BasicEffect`
geometry, same class of gap as every other item #5 sample.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** CNA has no `VertexPositionNormal` vertex type or matching normal-lit
GLSL shader yet, so `BasicEffect.EnableDefaultLighting()`/`PreferPerPixelLighting` has
nothing to render the cylinder chain with (see DEFERRED.md item #5's full text for
the engine-level detail).

**Tracked in:** DEFERRED.md item #5.

## Secondary asset work (not a hard blocker, standard conversion): `cylinder.x`
The cylinder model is a single static mesh (no independent per-part bones — `IKSample.cs`
never does a `Bones["..."]`-style lookup on it; the CCD chain's per-link transforms are
computed entirely in game code and passed in as the `world` matrix per draw call), so
once item #5 is resolved this only needs the ordinary `.x` → `.model.json` conversion
already proven for CameraShake/PerformanceMeasuring — see DEFERRED.md item #6 (the
"static geometry, conversion-only" case, not the rigid multi-bone case that blocks
SimpleAnimation/SplitScreen).

## Not blocked (confirmed while investigating): Avatar rendering (`AvatarRenderer`)
`IKSample.cs`'s second IK demo drives an `AvatarRenderer`/`AvatarDescription`/
`AvatarBone`/`AvatarExpression` (Xbox LIVE avatar) chain (`LoadAvatar()`,
`UpdateAvatarIK()`, `DrawAvatar()`). This looked like it might be an additional,
deeper, untracked gap (on top of item #5) the way CustomModelEffect's content-pipeline
processor is — but CNA already has a real, non-stub `GamerServices::AvatarRenderer`
implementation (`cna/src/Microsoft/Xna/Framework/GamerServices/AvatarRenderer.cpp`,
197 lines: real 71-bone avatar skeleton parent-index table, real bind-pose/world/view/
projection handling), plus `AvatarDescription`, `AvatarBone`, `AvatarExpression`, and
sibling avatar types. It faithfully reproduces genuine upstream XNA behavior: neither
`AvatarRenderer` constructor ever actually reads its `AvatarDescription` argument, so
every instance ends up permanently `AvatarRendererState::Unavailable` (never becomes
`Ready`) absent a real signed-in Xbox LIVE/Games-for-Windows-Live avatar — exactly
what `IKSample.cs` itself already defensively checks for
(`if (avatarRenderer.State != AvatarRendererState.Ready) { isAvatarInitialized = false; }`
in `UpdateAvatarIK()`, and `DrawAvatar()` guards its post-draw overlay work the same
way). So the avatar half of the sample is expected to behave identically to a real
Windows XNA build with no Live avatar signed in (i.e. no visible avatar), which is not
a CNA gap — it is not tracked as a blocker here.
