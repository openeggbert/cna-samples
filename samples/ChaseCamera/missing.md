# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `ChaseCamera.htm`) so the CNA-side blocker is documented in the same place a
future porting session will look. No `src/`/`CMakeLists.txt` exist yet — see
CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA gap
below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/ChaseCamera_4_0/
{ChaseCamera/{ChaseCamera.cs, ChaseCameraGame.cs, Ship.cs},
ChaseCameraContent/{Ship.fbx, Ground.x, ShipDiffuse.tga, Checker.bmp,
gameFont.spritefont}}`. The sample is a spring-physics chase camera following a ship
model flying over a checkered ground plane.

## Blocker: BasicEffect per-vertex lighting (`EnableDefaultLighting`)
**XNA behaviour:** `ChaseCameraGame.cs`'s `DrawModel()` helper (used for both the ship
and the ground) calls `effect.EnableDefaultLighting()` at line 245 on every
`BasicEffect` in each mesh before drawing it (confirmed in-session):
```csharp
foreach (BasicEffect effect in mesh.Effects)
{
    effect.EnableDefaultLighting();
    effect.World = transforms[mesh.ParentBone.Index] * world;
    effect.View = camera.View;
    effect.Projection = camera.Projection;
}
```
Both models (`shipModel`, `groundModel`) are drawn through this same lit-`BasicEffect`
path — same class of gap as every other item #5 sample.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** CNA has no `VertexPositionNormal` vertex type or matching normal-lit
GLSL shader yet, so `BasicEffect.EnableDefaultLighting()` has nothing to render either
model with (see DEFERRED.md item #5's full text for the engine-level detail).

**Tracked in:** DEFERRED.md item #5.

## Secondary asset work (not a hard blocker, standard conversion): `Ship.fbx` / `Ground.x`
`DrawModel()` uses the generic `transforms[mesh.ParentBone.Index]` /
`CopyAbsoluteBoneTransformsTo` pattern, but only to combine a *static* bone hierarchy
with a single shared `world` matrix per model — neither model's code ever looks up an
individual bone by name (no `Bones["..."]`-style per-part lookup like `Tank.cs`), so
this is the ordinary single/static-hierarchy case DEFERRED.md item #6 already lists as
conversion-only (no CNA code change needed) — `ChaseCamera` is in fact named directly
in that item's "conversion-only" blocked-samples list, alongside CameraShake and
BloomSample's tank.
