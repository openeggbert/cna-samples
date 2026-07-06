# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `Particle3D.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/Particles3DSample_4_0/Particle3DSample/ParticleSystem.cs`.
This is the "Particle3DSample" particle engine — five `ParticleSystem` subclasses
(`ExplosionParticleSystem`, `ExplosionSmokeParticleSystem`, `FireParticleSystem`,
`ProjectileTrailParticleSystem`, `SmokePlumeParticleSystem`) all share the same
base class and the same shader; the XmlParticles sample (`samples/XmlParticles/`
in this repo) is the *same underlying engine*, just with per-system settings
loaded from external XML instead of hardcoded in C# subclasses — see that
sample's `missing.md` for the parts specific to it.

## Blocker: shared custom `ParticleEffect.fx` HLSL shader

**XNA behaviour:** Every particle system loads the same custom effect
(`ParticleSystem.cs:229`, `LoadParticleEffect()`: `Effect effect =
content.Load<Effect>("ParticleEffect")`, then `.Clone()`s it per-system so each
system can set its own parameters without stomping the shared instance). The
shader (`Particle3DSample/Content/ParticleEffect.fx`) animates every particle
**entirely on the GPU**: the vertex shader (`ParticleVertexShader`) takes a
per-particle start position/velocity/creation-time/random values
(`ParticleVertex` struct: `Corner`, `Position`, `Velocity`, `Random`, `Time`)
and integrates constant-acceleration motion (`ComputeParticlePosition`,
gravity + velocity-decay-to-`EndVelocity`), size-over-lifetime interpolation
(`ComputeParticleSize`), a fade-in/fade-out alpha curve
(`ComputeParticleColor`), and per-particle rotation
(`ComputeParticleRotation`, a 2x2 rotation matrix applied to the billboard
corner) — all driven purely by `CurrentTime` and the per-vertex data, with no
per-frame CPU particle updates. The pixel shader just samples the particle
texture and multiplies by the computed color.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** `ParticleEffect.fx` is genuine custom HLSL (vs 2.0/ps 2.0,
GPU-driven particle animation) that must be hand-translated to GLSL plus a
`.shader.json` descriptor before `Content.Load<Effect>` (already implemented
in CNA for `.shader.json`-described effects) can serve it. The
`DynamicVertexBuffer`/circular-queue particle-slot bookkeeping in
`ParticleSystem.cs` is ordinary C++-portable logic and is not itself blocked.

**Tracked in:** DEFERRED.md item #11.
