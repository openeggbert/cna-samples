# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `NonPhotoRealistic.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/NonPhotoRealisticSample_4_0/NonPhotoRealistic/Game.cs`.

## Blocker: custom HLSL `.fx` shaders (cel/cartoon shading + edge-detect post-process)

**XNA behaviour:** The sample renders a ship model with cartoon/cel-shaded lighting
and ink-outline edges. Two custom HLSL effects are loaded directly at runtime:
- `Content/CartoonEffect.Fx` — loaded via `Content.Load<Effect>("CartoonEffect")`
  (`Game.cs:97`) and swapped onto every mesh part of the loaded `Ship` model via
  `ChangeEffectUsedByModel(model, cartoonEffect)` (`Game.cs:99`, helper defined at
  `Game.cs:132`), producing the toon-shaded diffuse lighting and a normal/depth
  render for edge detection.
- `Content/PostprocessEffect.Fx` — loaded via
  `Content.Load<Effect>("PostprocessEffect")` (`Game.cs:93`) and applied as a
  full-screen pass over two custom `RenderTarget2D`s (`sceneRenderTarget` and
  `normalDepthRenderTarget`, created at `Game.cs:104-111`) to draw the pencil-sketch
  hatching and ink outlines. `RenderTarget2D` is already supported in CNA, so that
  part is not a blocker.

**CNA port behaviour:** N/A yet (not ported). `Content.Load<Effect>()` for a
`.shader.json`-described effect already works in CNA, but no GLSL translation of
`CartoonEffect.Fx` or `PostprocessEffect.Fx` exists, and there is no tooling to
produce one.

**Root cause:** Both `.fx` files are custom HLSL shaders (toon-shading material
effect + full-screen edge-detect/sketch post-process effect) with no CNA/GLSL
equivalent yet. Converting either requires hand-translating HLSL to GLSL and
authoring a `.shader.json` descriptor per effect — real tooling work in the `cna`
repo, not something that can be worked around per-sample.

**Tracked in:** DEFERRED.md item #11
