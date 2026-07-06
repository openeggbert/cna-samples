# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `VertexLighting.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/VertexLightingSample_4_0/VertexLighting/VertexLighting.cs`.

## Blocker: custom HLSL `.fx` shaders (flat-shaded vs. smooth vertex lighting comparison)

**XNA behaviour:** The sample contrasts flat (faceted) shading against smooth
per-vertex lighting, both loaded directly at runtime:
```csharp
noLightingEffect     = Content.Load<Effect>("FlatShaded");      // VertexLighting.cs:115
vertexLightingEffect = Content.Load<Effect>("VertexLighting");  // VertexLighting.cs:116
```
backed by `Content/FlatShaded.fx` (a minimal shader with no lighting, used to show
raw per-face normals) and `Content/VertexLighting.fx` (three-light Lambertian
diffuse + specular computed per-vertex) — the sample toggles between the two
techniques/effects at runtime to illustrate the difference vertex lighting makes.

**CNA port behaviour:** N/A yet (not ported). `Content.Load<Effect>()` for a
`.shader.json`-described effect already works in CNA, but no GLSL translation of
either `.fx` file exists, and there is no tooling to produce one.

**Root cause:** Both `.fx` files are custom HLSL vertex+pixel shader pairs with no
CNA/GLSL equivalent yet. Converting either requires hand-translating HLSL to GLSL
and authoring a `.shader.json` descriptor per effect — real tooling work in the
`cna` repo, not something that can be worked around per-sample.

**Tracked in:** DEFERRED.md item #11
