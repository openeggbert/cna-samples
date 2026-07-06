# Missing / Differences from XNA 4.0 original

**Status: UNBLOCKED, not yet ported — corrected 2026-07-06.** The blocker below was
accurate when first written this session, but a live build+run of `cna_test_
easygl_basiceffect_combinations` right after found CNA's `VertexPositionNormalTexture`
lit path (exactly what `Content.Load<Model>("terrain")` produces) already works —
case "(e) Directional lighting" passes (exit code 0). DEFERRED.md item #5 is marked
resolved for `Model`-based samples. No CNA gap remains; this is now a normal,
straightforward porting candidate. (Kept the original write-up below for reference.)

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

**Root cause (historical):** was a missing lit-shader path for `VertexPositionNormalTexture`
in CNA; now resolved (see Status note above).

**Tracked in:** DEFERRED.md item #5 (resolved)
