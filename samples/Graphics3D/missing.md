# Missing / Differences from XNA 4.0 original

**Status: UNBLOCKED, not yet ported — corrected 2026-07-06.** The blocker below was
accurate when first written this session, but a live build+run of `cna_test_
easygl_basiceffect_combinations` right after found CNA's `VertexPositionNormalTexture`
lit path (exactly what `content.Load<Model>("Models/spaceship")` produces) already
works — case "(e) Directional lighting" passes (exit code 0). DEFERRED.md item #5
is marked resolved for `Model`-based samples. No CNA gap remains; this is now a
normal, straightforward porting candidate. (Kept the original write-up below.)

Source: `/rv/tmp/XNAGameStudio/Samples/Graphics3DSample_4_0/Sample3DGraphics/Sample3DGraphics/Models/Spaceship.cs`.

## Blocker: `BasicEffect.LightingEnabled = true` with 3 directional lights on the spaceship model

**XNA behaviour:** `Spaceship.Draw()` (`Spaceship.cs:117-140`) loops over
`spaceshipModel.Meshes` / `mesh.Effects` (all **stock** `BasicEffect` instances) and
calls `SetEffectLights(effect, Lights)` (`Spaceship.cs:154-181`), which configures
three independent directional lights (`DirectionalLight0/1/2`, each with its own
diffuse/specular color and direction) and finishes with `effect.LightingEnabled = true;`
at `Spaceship.cs:183`. It also exposes a per-pixel-lighting toggle
(`effect.PreferPerPixelLighting = isPerPixelLightingEnabled;`, `Spaceship.cs:151`,
driven by `IsPerPixelLightingEnabled`, `Spaceship.cs:107-111`, and the on-screen
"Toggle Per-Pixel Lighting" button in `GameMain.cs:225`). Confirmed via direct source
audit: **zero custom `.fx` files anywhere in this sample** — the entire lighting demo
runs through the stock `BasicEffect`, not a hand-written shader.

**CNA port behaviour:** N/A yet (not ported). CNA's `BasicEffect` currently only
supports flat/unlit rendering with `VertexPositionColor` (as used by the Primitives3D
port); there is no `VertexPositionNormal` vertex struct and no per-vertex/per-pixel-lit
GLSL shader in the EasyGL backend, so neither `LightingEnabled` nor
`PreferPerPixelLighting` has anything to render with.

**Root cause (historical):** was a missing lit-shader path for `VertexPositionNormalTexture`
in CNA; now resolved (see Status note above). Note: `PreferPerPixelLighting` may
still be a no-op if CNA's implementation doesn't distinguish per-vertex vs.
per-pixel — verify and note in missing.md if so once ported.

**Tracked in:** DEFERRED.md item #5 (resolved)
