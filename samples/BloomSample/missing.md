# Missing / Differences from XNA 4.0 original

## Not ported — blocked by custom shaders, RenderTarget2D, and Model loading

**XNA behaviour:** Renders a 3D scene (spinning tank model + sunset background) and applies
a multi-pass Gaussian bloom post-process using three custom HLSL pixel shaders
(`BloomExtract.fx`, `GaussianBlur.fx`, `BloomCombine.fx`) and two intermediate
`RenderTarget2D` half-resolution buffers.  A `DrawableGameComponent` (`BloomComponent`)
encapsulates the bloom pipeline and can be toggled on/off at runtime (B key).

**CNA port behaviour:** Not implemented. No source files exist yet.

**Root cause:** Multiple CNA features are missing:
1. Custom user Effect (pixel shader) loading — DEFERRED.md item #11
2. `RenderTarget2D` / off-screen render targets — DEFERRED.md item #12
3. `Content.Load<Model>()` (tank.fbx) — DEFERRED.md item #6
4. `DrawableGameComponent` / Game.Components — not yet in CNA

**Tracked in:** DEFERRED.md items #6, #11, #12
