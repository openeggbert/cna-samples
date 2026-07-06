# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `Billboard.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/BillboardSample_4_0/Billboard/Billboard.cs` and
`/rv/tmp/XNAGameStudio/Samples/BillboardSample_4_0/BillboardPipeline/VegetationProcessor.cs`.

## Blocker: custom `Billboard.fx` HLSL shader

**XNA behaviour:** A custom content pipeline processor (`VegetationProcessor.cs`)
generates camera-facing billboard geometry (grass/trees) for a landscape model
and points every generated billboard's `EffectMaterialContent` at a hand-written
`Billboard.fx` (`VegetationProcessor.cs:219`: `Path.Combine(directory, "Billboard.fx")`,
`:221`: `material.Effect = new ExternalReference<EffectContent>(effectFilename)`).
The shader (`Billboard/Content/Billboard.fx`) does real per-vertex work beyond
what `BasicEffect`/`AlphaTestEffect` can express: it billboards each quad to face
the camera using `View._m02_m12_m22`, squishes/flips alternating billboards using
a per-vertex "Random" texture-coordinate channel, animates a sine-wave wind sway
(`WindDirection`, `WindWaveSize`, `WindRandomness`, `WindSpeed`, `WindAmount`,
`WindTime`), applies simple directional + ambient lighting, and does an alpha-test
`clip()` in the pixel shader driven by `AlphaTestDirection`/`AlphaTestThreshold`
(used to two-pass render opaque then transparent billboards — see
`Billboard.cs:151-175`, which sets `AlphaTestDirection` to `1f` then `-1f` across
two mesh-effects loops). The landscape's non-billboard ground mesh uses stock
`BasicEffect` (`Billboard.cs:105-116`) and is not blocked.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** `Billboard.fx` is genuine custom HLSL (vs 2.0/ps 2.0) that must be
hand-translated to GLSL plus a `.shader.json` descriptor before
`Content.Load<Effect>` (already implemented in CNA for `.shader.json`-described
effects) can serve it. The billboard-quad-generation math in
`VegetationProcessor.cs` itself is ordinary content-pipeline C# and is not
blocked; it is the shader consuming that geometry that is missing.

**Tracked in:** DEFERRED.md item #11.
