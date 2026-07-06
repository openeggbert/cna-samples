# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `Distortion.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/DistortionSample_4_0/Distortion/`.

## Blocker: custom HLSL `.fx` shaders (post-process distortion + material)

**XNA behaviour:** The sample renders a 3D scene of "distorter" objects, then applies
a full-screen post-process distortion/heat-haze pass driven by two custom HLSL
effects:
- `Content/Distort.fx` — a full-screen pixel shader loaded directly at runtime via
  `Game.Content.Load<Effect>("Distort")` (`DistortionComponent.cs:77`), which selects
  between its `Distort` and `DistortBlur` techniques (`DistortionComponent.cs:78-79`)
  to warp the rendered scene using a displacement map.
- `Content/Distorters.fx` — a material effect assigned to the distorter models at
  content-build time by `DistorterMaterialProcessor.cs:39-41`
  (`effect.Effect = new ExternalReference<EffectContent>("Distorters.fx")`), which
  renders each distorter's displacement geometry into an offscreen buffer.

A `DrawableGameComponent` (`DistortionComponent`) encapsulates the whole pipeline;
`DrawableGameComponent`/`Game.Components` are already implemented in CNA
(`cna/include/Microsoft/Xna/Framework/DrawableGameComponent.hpp`), so that part is
not a blocker.

**CNA port behaviour:** N/A yet (not ported). `Content.Load<Effect>()` for a
`.shader.json`-described effect already works in CNA, but no GLSL translation of
`Distort.fx` or `Distorters.fx` exists, and there is no tooling to produce one.

**Root cause:** Both `.fx` files are custom HLSL shaders (one runtime post-process
effect, one build-time material effect) with no CNA/GLSL equivalent yet. Converting
either requires hand-translating HLSL to GLSL and authoring a `.shader.json`
descriptor per effect — real tooling work in the `cna` repo, not something that can
be worked around per-sample.

**Tracked in:** DEFERRED.md item #11
