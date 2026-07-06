# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `ShatterEffect.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/ShatterEffectSample_4_0/ShatterEffect/ShatterEffectGame.cs`
and `.../ShatterProcessor/ShatterProcessor.cs`.

## Blocker: custom `ShatterEffect.fx` HLSL shader

**XNA behaviour:** A custom content processor (`ShatterProcessor.cs`) splits
every model triangle into its own disconnected copy and points the resulting
material at a hand-written effect (`ShatterProcessor.cs:179`: `new
ExternalReference<EffectContent>("shatterEffect.fx")`). The shader
(`ShatterEffect/Content/ShatterEffect.fx`) does the actual "shattering" entirely
on the GPU: its vertex shader (`ShatterVS`) builds a per-triangle
yaw/pitch/roll rotation matrix (`CreateYawPitchRollMatrix`) from a
per-triangle random `RotationalVelocity` vertex channel scaled by a
`RotationAmount` parameter, rotates each vertex about its triangle's
precomputed center, translates it outward along its normal by
`TranslationAmount`, and drops it via `time*time * 200` for a falling arc —
then computes per-vertex diffuse lighting from a `lightPosition`. Its pixel
shader (`PhongPS`) adds a specular Phong term (`specularColor`,
`specularPower`, `eyePosition`) on top of the textured diffuse color. The game
side (`ShatterEffectGame.cs:146-162`, `SetupEffect`) drives all of this every
frame by setting `TranslationAmount`, `RotationAmount`, `time`,
`WorldViewProjection`, `World`, `eyePosition`, `lightPosition`,
`ambientColor`/`diffuseColor`/`specularColor`, `specularPower` as effect
parameters directly (`part.Effect.Parameters[...]`) — there is no stock XNA
effect that can express this per-triangle procedural shatter/rotate/fall
animation plus Phong specular.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** `ShatterEffect.fx` is genuine custom HLSL (vs 2.0/ps 2.0) that
must be hand-translated to GLSL plus a `.shader.json` descriptor before
`Content.Load<Effect>` (already implemented in CNA for `.shader.json`-described
effects) can serve it. The triangle-splitting/per-triangle-random-value
generation in `ShatterProcessor.cs` is ordinary content-pipeline C# math and is
not itself blocked; it is the shader consuming its output vertex channels that
is missing.

**Tracked in:** DEFERRED.md item #11.
