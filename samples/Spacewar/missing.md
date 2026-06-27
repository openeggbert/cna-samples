# Missing / Differences from XNA 4.0 original

## Not ported — blocked by multiple CNA gaps

**XNA behaviour:** Full 2-player space combat game with two visual styles:
- **Evolved** — 3D ship/asteroid models, custom HLSL shaders, cubemap reflections,
  RenderTarget2D offscreen scaling pipeline, XACT cue-based audio.
- **Retro** — procedural vector shapes, starfield, sun rendered via `BasicEffectShape`.
Both styles share menu screens (TitleScreen, SelectionScreen, ShipUpgradeScreen,
VictoryScreen), a scene-graph system, camera, particles, and XACT SoundBank/WaveBank.

**CNA port behaviour:** Not implemented. No source files exist yet.

**Root cause:** Multiple CNA features are missing:
1. `Content.Load<Model>()` — Evolved ships/asteroids use `.x` / `.fbx` models (DEFERRED.md #6)
2. Custom user Effect / HLSL shaders — 5× `.fx` files (backdrop, ship, simple, sun, simplescreen) (DEFERRED.md #11)
3. `RenderTarget2D` — offscreen draw-scaling pipeline (DEFERRED.md #12)
4. XACT SoundBank/WaveBank compiled output — source `.xap` has no pre-built `.xsb`/`.xwb` in the repo

**Tracked in:** DEFERRED.md items #6, #11, #12
