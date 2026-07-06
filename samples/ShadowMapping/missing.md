# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `ShadowMapping.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/ShadowMappingSample_4_0/ShadowMapping/ShadowMapping.cs`.

## Blocker: custom HLSL `.fx` shader (shadow-map generation + shadowed scene rendering)

**XNA behaviour:** Renders a `dude` model and a `grid` model, both lit with real-time
shadow mapping via a single custom effect. As with NormalMapping, the effect is not
loaded directly via `Content.Load<Effect>` in game code — it is attached to both
models at content-build time by
`ShadowMappingContent.contentproj:71-72,80-81`:
```xml
<Processor>CustomEffectModelProcessor</Processor>
<ProcessorParameters_CustomEffect>DrawModel.fx</ProcessorParameters_CustomEffect>
```
(`CustomEffectMaterialProcessor.cs:47-51` performs the actual swap-in of
`Content/DrawModel.fx` as each mesh part's `EffectMaterialContent`). At runtime,
`DrawModel()` (`ShadowMapping.cs:263`) selects between the effect's shadow-map-pass
and main-scene-pass techniques and sets its parameters, e.g.:
```csharp
effect.CurrentTechnique = effect.Techniques[techniqueName];   // ShadowMapping.cs:277
effect.Parameters["LightViewProj"].SetValue(lightViewProjection); // ShadowMapping.cs:281
effect.Parameters["ShadowMap"].SetValue(shadowRenderTarget);      // ShadowMapping.cs:285
```
`RenderTarget2D` (used for the shadow map render target) is already supported in
CNA, so that part is not a blocker.

**CNA port behaviour:** N/A yet (not ported). `Content.Load<Effect>()` for a
`.shader.json`-described effect already works in CNA, but no GLSL translation of
`DrawModel.fx` exists, and there is no tooling to produce one.

**Root cause:** `DrawModel.fx` is a custom HLSL effect with multiple techniques
(depth-only shadow-map pass, main shadowed-scene pass) and no CNA/GLSL equivalent
yet. Converting it requires hand-translating HLSL to GLSL — including the
depth-comparison shadow-map sampling logic — and authoring a `.shader.json`
descriptor per technique/pass; real tooling work in the `cna` repo, not something
that can be worked around per-sample.

**Tracked in:** DEFERRED.md item #11
