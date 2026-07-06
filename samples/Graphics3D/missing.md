# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `3DGraphics.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

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

**Root cause:** Missing `VertexPositionNormal` struct + normal-lit shader in CNA
(DEFERRED.md item #5), not a missing asset-conversion pipeline.

**Tracked in:** DEFERRED.md item #5
