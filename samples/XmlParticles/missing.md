# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `XmlParticles.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/XmlParticles_4_0/XmlParticles/Particle3DSample/ParticleSystem.cs`.
This is the **same "Particle3DSample" particle engine** as `samples/Particles3D/`
in this repo (identical `ParticleVertex`/`DynamicVertexBuffer`/circular-queue
mechanics and, byte-for-byte, the same `LoadParticleEffect()` helper — compare
`ParticleSystem.cs:229` in Particles3D vs `ParticleSystem.cs:218` here). The
only functional addition XmlParticles makes on top of that engine is replacing
the five hardcoded `ParticleSettings` subclasses with a single concrete
`ParticleSystem` that loads its tunables from an external XML file per
instance (`Game.cs:95`: `new ParticleSystem(this, Content, "ExplosionSettings")`;
`ParticleSystem.cs:176`: `settings = content.Load<ParticleSettings>(settingsName)`
against XML assets `Content/ExplosionSettings.xml`, `FireSettings.xml`,
`ExplosionSmokeSettings.xml`, `ProjectileTrailSettings.xml`,
`SmokePlumeSettings.xml`).

## Not itself blocked: the XML settings loader

The XML files are the stock XNA content-pipeline `XnaContent`/`IntermediateSerializer`
format (a flat `<Asset Type="Particle3DSample.ParticleSettings">` element whose
child tags map 1:1 onto public fields — see `Content/ExplosionSettings.xml`,
e.g. `<TextureName>explosion</TextureName>`, `<Duration>PT2S</Duration>`,
`<MinColor>FF808080</MinColor>`). In real XNA this is compiled to a `.xnb` at
build time, but the format itself is simple enough to parse directly at
runtime in a CNA port — CNA/std C++ can already parse simple XML, and this
repo has a working precedent in `samples/RolePlayingGame/src/Xml/XmlNode.hpp`
(used to load that sample's `Content/*.xml` quest/map data). **This part of
XmlParticles is not blocked** and would need no new CNA tooling.

## Blocker: shared custom `ParticleEffect.fx` HLSL shader

**XNA behaviour:** Identical to `samples/Particles3D/` — see that sample's
`missing.md` for the full shader description (`ParticleEffect.fx`: GPU-driven
particle motion/size/color/rotation integrated entirely in the vertex shader
from `CurrentTime` and per-vertex start data, no per-frame CPU particle
updates).

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** `ParticleEffect.fx` (`Content/ParticleEffect.fx`, identical to
the Particles3D sample's copy) is genuine custom HLSL that must be
hand-translated to GLSL plus a `.shader.json` descriptor before
`Content.Load<Effect>` (already implemented in CNA for `.shader.json`-described
effects) can serve it. This is the **only** real blocker for this sample — the
XML-driven settings loading described above is not blocked.

**Tracked in:** DEFERRED.md item #11.
