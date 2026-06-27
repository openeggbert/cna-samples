# Missing / Differences from XNA 4.0 original

## Not ported — blocked by custom shaders, RenderTarget2D, and Model loading

**XNA behaviour:** Renders a 3D scene (spinning tank model + sunset background) and applies
a multi-pass Gaussian bloom post-process using three custom HLSL pixel shaders
(`BloomExtract.fx`, `GaussianBlur.fx`, `BloomCombine.fx`) and two intermediate
`RenderTarget2D` half-resolution buffers.  A `DrawableGameComponent` (`BloomComponent`)
encapsulates the bloom pipeline and can be toggled on/off at runtime (B key).

**CNA port behaviour:** Not implemented. No source files exist yet.

**Root cause:** Asset conversion work required:
1. HLSL `.fx` shaders must be rewritten as GLSL `.shader.json` — DEFERRED.md item #11
2. `tank.fbx` model must be converted to `.model.json` — DEFERRED.md item #6
3. `DrawableGameComponent` / Game.Components — not yet in CNA
4. `RenderTarget2D` is supported in CNA; no blocker there.

**Tracked in:** DEFERRED.md items #6, #11, #12
