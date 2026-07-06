# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `LensFlare.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/LensFlareSample_4_0/LensFlare/Game.cs`.

## Blocker: `BasicEffect.LightingEnabled = true` on the terrain model

**XNA behaviour:** Loads a static `terrain` model (`Content.Load<Model>("terrain")`,
`Game.cs:61`) and renders it every frame with per-vertex lighting turned on via the
**stock** `BasicEffect` (`foreach (BasicEffect effect in mesh.Effects)`, `Game.cs:104-111`,
followed immediately by `effect.LightingEnabled = true;` at `Game.cs:112`, plus
`AmbientLightColor`/`DirectionalLight0` setup and `FogEnabled`). This lets the sample's
`LensFlareComponent` use an occlusion query to detect whether the sun is hidden behind
the lit landscape. Confirmed via direct source audit: **zero custom `.fx` files
anywhere in this sample** (`LensFlareComponent.cs` only manipulates render states and
issues the occlusion query, no shader code) — this needs no shader-pipeline work at
all, unlike the rest of Phase 3/4.

**CNA port behaviour:** N/A yet (not ported). CNA's `BasicEffect` currently only
supports flat/unlit rendering with `VertexPositionColor` (as used by the Primitives3D
port) — there is no `VertexPositionNormal` vertex struct and no per-vertex-lit GLSL
shader in the EasyGL backend, so `LightingEnabled = true` has nothing to render with.

**Root cause:** Missing `VertexPositionNormal` struct + normal-lit shader in CNA
(DEFERRED.md item #5), not a missing asset-conversion pipeline.

**Tracked in:** DEFERRED.md item #5
