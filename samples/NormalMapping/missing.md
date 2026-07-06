# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `NormalMappingEffect.htm`) so the CNA-side blocker is documented where a
future porting session will look. No `src/`/`CMakeLists.txt` exist yet — see
CLAUDE.md's "Adding a new sample" steps for what's still needed once the gap below
is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/NormalMappingSample_4_0/NormalMappingEffect/NormalMappingEffect.cs`.

## Blocker: custom HLSL `.fx` shader (per-pixel normal-mapped lighting)

**XNA behaviour:** Renders a model lit by a custom per-pixel normal-mapping effect
defined in `Content/NormalMapping.fx`. Unlike some other samples in this batch, the
effect is not loaded directly via `Content.Load<Effect>` in game code — it is
attached to the model at content-build time by
`NormalMappingModelProcessor.cs:178-180`:
```csharp
EffectMaterialContent normalMappingMaterial = new EffectMaterialContent();
normalMappingMaterial.Effect = new ExternalReference<EffectContent>
    (Path.Combine(directory, "NormalMapping.fx"));
```
At runtime the game only sets parameters on the resulting `Effect` instances found
in `mesh.Effects` (`NormalMappingEffect.cs:77-84` for lighting parameters,
`NormalMappingEffect.cs:157-164` for transform/light-position parameters) — the
shader math itself (tangent-space normal mapping, specular highlight) lives entirely
in the `.fx` file.

**CNA port behaviour:** N/A yet (not ported). `Content.Load<Effect>()` for a
`.shader.json`-described effect already works in CNA, but no GLSL translation of
`NormalMapping.fx` exists, and there is no tooling to produce one.

**Root cause:** `NormalMapping.fx` is a custom HLSL vertex+pixel shader pair (tangent
frame construction, normal-map sampling, per-pixel Blinn-Phong-style lighting) with
no CNA/GLSL equivalent yet. Converting it requires hand-translating HLSL to GLSL and
authoring a `.shader.json` descriptor — real tooling work in the `cna` repo, not
something that can be worked around per-sample. Note also that this sample's
`NormalMappingModelProcessor`/`NormalMapTextureProcessor` pipeline assumes a custom
content-build step (tangent-frame generation, normal-map format conversion) that a
future port would need to bake into the converted asset ahead of time, since CNA has
no content-pipeline build step of its own.

**Tracked in:** DEFERRED.md item #11
