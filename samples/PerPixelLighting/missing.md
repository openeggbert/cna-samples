# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `PerPixelLighting.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/PerPixelLightingSample_4_0/PerPixelLighting/PerPixelLighting.cs`.

## Blocker: custom HLSL `.fx` shaders (per-vertex vs. per-pixel lighting comparison)

**XNA behaviour:** The sample's whole point is to contrast two custom lighting
effects side by side, both loaded directly at runtime:
```csharp
effects[0] = Content.Load<Effect>("VertexLighting");   // PerPixelLighting.cs:148
effects[1] = Content.Load<Effect>("PerPixelLighting");  // PerPixelLighting.cs:149
```
backed by `Content/VertexLighting.fx` and `Content/PerPixelLighting.fx`
respectively — the former does Lambertian diffuse + specular lighting in the vertex
shader (interpolated per-pixel), the latter recomputes full per-pixel lighting in
the pixel shader, and the sample toggles between them at runtime to demonstrate the
visual quality difference (faceted vs. smooth specular highlights).

**CNA port behaviour:** N/A yet (not ported). `Content.Load<Effect>()` for a
`.shader.json`-described effect already works in CNA, but no GLSL translation of
either `.fx` file exists, and there is no tooling to produce one.

**Root cause:** Both `.fx` files are custom HLSL vertex+pixel shader pairs with no
CNA/GLSL equivalent yet. Converting either requires hand-translating HLSL to GLSL
and authoring a `.shader.json` descriptor per effect — real tooling work in the
`cna` repo, not something that can be worked around per-sample.

**Tracked in:** DEFERRED.md item #11
