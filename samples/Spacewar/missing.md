# Missing / Differences from XNA 4.0 original

## Not ported — blocked by multiple CNA gaps

**XNA behaviour:** Full 2-player space combat game with two visual styles:
- **Evolved** — 3D ship/asteroid models, custom HLSL shaders, cubemap reflections,
  RenderTarget2D offscreen scaling pipeline, XACT cue-based audio.
- **Retro** — procedural vector shapes, starfield, sun rendered via `BasicEffectShape`.
Both styles share menu screens (TitleScreen, SelectionScreen, ShipUpgradeScreen,
VictoryScreen), a scene-graph system, camera, particles, and XACT SoundBank/WaveBank.

**CNA port behaviour:** Not implemented. No source files exist yet.

**Root cause:** Asset conversion work required:
1. Evolved ships/asteroids (`.x`/`.fbx` models) must be converted to `.model.json` (DEFERRED.md #6)
2. 5× HLSL `.fx` shaders must be rewritten as GLSL `.shader.json` (DEFERRED.md #11)
3. XACT `.xap` source must be compiled to `.xsb`/`.xwb` — the source repo has only the
   XACT project, not the compiled SoundBank/WaveBank output files
4. `RenderTarget2D` is supported in CNA (no blocker); `DrawableGameComponent`/`Game.Components`
   not yet in CNA but can be managed manually as in ParticleSample

**Tracked in:** DEFERRED.md items #6, #11, #12
