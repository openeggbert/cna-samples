# Missing / Differences from XNA 4.0 original

**Status: UNBLOCKED, not yet ported — corrected 2026-07-06.** The blocker below was
accurate when first written this session, but a live build+run of `cna_test_
easygl_basiceffect_combinations` right after found CNA's `VertexPositionNormalTexture`
lit path already works — case "(e) Directional lighting" passes (exit code 0).
DEFERRED.md item #5 is marked resolved for `Model`-based samples. No CNA gap
remains; this is now a normal, straightforward porting candidate. (Kept the
original write-up below.)

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

**Root cause (historical):** was a missing lit-shader path for `VertexPositionNormalTexture`
in CNA; now resolved (see Status note above).

**Tracked in:** DEFERRED.md item #5 (resolved).

## Secondary asset work (not a hard blocker, standard conversion): `Ship.fbx` / `Ground.x`
`DrawModel()` uses the generic `transforms[mesh.ParentBone.Index]` /
`CopyAbsoluteBoneTransformsTo` pattern, but only to combine a *static* bone hierarchy
with a single shared `world` matrix per model — neither model's code ever looks up an
individual bone by name (no `Bones["..."]`-style per-part lookup like `Tank.cs`), so
this is the ordinary single/static-hierarchy case DEFERRED.md item #6 already lists as
conversion-only (no CNA code change needed) — `ChaseCamera` is in fact named directly
in that item's "conversion-only" blocked-samples list, alongside CameraShake and
BloomSample's tank.
