# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `CustomModelEffect.htm`) so the CNA-side blockers are documented in the same
place a future porting session will look. No `src/`/`CMakeLists.txt` exist yet — see
CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA gaps
below are fixed. **This sample is deeper and different from SimpleAnimation /
CustomModelClass / InverseKinematics / ChaseCamera** — it is not just a lighting or
bone-hierarchy gap; it needs a capability CNA's asset pipeline has no equivalent of
at all (see Blocker 2).

Source: `/rv/tmp/XNAGameStudio/Samples/CustomModelEffectSample_4_0/
{CustomModelEffect/CustomModelEffect.cs, CustomModelEffect/Content/EnvironmentMap.fx,
CustomModelEffectPipeline/{EnvironmentMappedMaterialProcessor.cs, CubemapProcessor.cs,
EnvironmentMappedModelProcessor.cs}}`. The sample renders a spinning saucer model
(`saucer.fbx`) with a chrome-like reflective material: a custom vertex/pixel shader
blends the saucer's own diffuse texture with a reflection cubemap (generated at
content-build time from a single flat photo, `seattle.bmp`), using a Fresnel term so
the surface looks more reflective at grazing angles.

## Blocker 1: custom HLSL shader (`EnvironmentMap.fx`)
**XNA behaviour:** `Content/EnvironmentMap.fx` is a hand-written HLSL `vs_2_0`/`ps_2_0`
effect (`VertexShaderFunction`/`PixelShaderFunction`) that computes a per-vertex
reflection vector and Fresnel coefficient, then in the pixel shader samples both a
2D `Texture` and a `textureCUBE` `EnvironmentMap`, `lerp`s between them by the Fresnel
term, and multiplies by a simple directional+ambient lighting term.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** CNA's `Content.Load<Effect>()` / `.shader.json` custom-effect pipeline
does exist (proven working elsewhere), but `EnvironmentMap.fx`'s HLSL logic has not
been translated to GLSL, and no `.shader.json` descriptor exists for it yet.

**Tracked in:** DEFERRED.md item #11.

## Blocker 2: build-time content-pipeline processor chain (no CNA equivalent exists)
**XNA behaviour:** Unlike a sample that just ships a custom `.fx` file, this sample's
own `CustomModelEffectPipeline` project *extends the MSBuild-time content pipeline
itself* with three chained custom processors:
1. `EnvironmentMappedModelProcessor : ModelProcessor` — overrides `ConvertMaterial()`
   to redirect every material on the model through the next processor below, passing
   an `EnvironmentMap` filename as an opaque processor parameter.
2. `EnvironmentMappedMaterialProcessor : MaterialProcessor` — overrides `Process()` to
   build an `EffectMaterialContent` pointing at `EnvironmentMap.fx` instead of the
   default `BasicEffect`-generating material, copies across the model's own diffuse
   texture, and overrides `BuildTexture()` so that specifically the `"EnvironmentMap"`
   texture slot is routed through processor 3 below (all other textures get normal
   default processing).
3. `CubemapProcessor : ContentProcessor<TextureContent, TextureCubeContent>` — turns
   an arbitrary flat 2D image (`seattle.bmp`) into a 6-face reflection cubemap: mirrors
   the source left-to-right to avoid a seam, copies out the four side faces, and
   *synthesizes* the top/bottom faces by folding/stretching four trapezoidal flaps
   from the top/bottom strip of the source image inward (`ScaleTrapezoid`) at 4x
   supersampling, then box-blurs the result to hide the fold seams
   (`BlurCubemapFace`/`ApplyBlurPass`), generates mipmaps, and compresses to DXT1.

All three run automatically as part of `saucer.fbx`'s MSBuild content-build step —
`Content.Load<Model>("saucer")` at runtime just gets back an ordinary `Model` whose
mesh materials are already-built `Effect`s with the reflection cubemap already baked
in as a texture parameter. There is no equivalent runtime API call the sample game
code makes; **all** of the interesting logic above runs at build time, invisibly, via
the `.contentproj`'s processor selection.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** CNA's entire asset story is "convert once, offline, with a standalone
tool, into a static runtime JSON/binary format" (`tools/obj2model.py`,
`tools/make_font.py`, `tools/gen_help_png.py`, etc.) — there is no pluggable,
MSBuild-time, C#-`ContentProcessor`-style extensibility point in CNA's pipeline at
all, and nothing resembling XNA's `ContentProcessor<TInput,TOutput>` /
`ContentProcessorContext.Convert`/`BuildAsset` chaining model. Porting this sample
faithfully would require inventing an equivalent (even if only informally, as a
one-off Python/C++ offline preprocessing script that performs the same three steps —
model-material-texture chaining ending in a synthesized DXT1(-or-equivalent) cubemap
— against `saucer.fbx` + `seattle.bmp`, producing a `.model.json` + cubemap file
CNA's runtime can load directly) rather than just converting one asset format to
another the way every other sample's asset work does.

**This gap is deeper than item #6 or item #11 alone.** It is explicitly *not* the same
as item #6 (static model conversion, which is pure format translation) or item #11
(shader conversion alone, Blocker 1 above) — it is the *meta*-capability of a
pluggable build-time content processor that item #6/#11 assume already produced
their inputs by some other means. No other Phase 3/4 sample audited so far has
needed this — most custom-`.fx` samples (BloomSample, NormalMapping, etc.) apply
their custom effect entirely at runtime via ordinary `Content.Load<Effect>()` calls
in game code, with no custom `ContentProcessor` involved.

**Tracked in:** DEFERRED.md item #18 (content-pipeline processor extensibility, added
specifically for this sample).
