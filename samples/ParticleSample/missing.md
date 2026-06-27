# Missing / Differences from XNA 4.0 original

## Additive blending for explosion particles not used
**XNA behaviour:** `ExplosionParticleSystem` uses `BlendState.Additive` giving the explosion a bright glowing look. `ExplosionSmokeParticleSystem` and `SmokePlumeParticleSystem` use `BlendState.AlphaBlend`.
**CNA port behaviour:** All three particle systems are drawn in one `SpriteBatch::Begin()/End()` block with default alpha blend. The explosion still fades in/out via alpha but lacks the additive glow.
**Root cause:** CNA Vulkan backend bug — only the last `SpriteBatch::Begin()/End()` pair per frame is rendered. Using separate Begin/End per system would cause all but the last to be discarded.
**Tracked in:** CNA issue (Vulkan multi-batch bug)

## DrawableGameComponent not used
**XNA behaviour:** Each `ParticleSystem` inherits from `DrawableGameComponent` and is registered in `Game.Components`. Update/Draw are called automatically via the component system.
**CNA port behaviour:** Particle systems are managed directly as `std::unique_ptr` members of the Game class, with Update/Draw called explicitly.
**Root cause:** CNA component system not confirmed to support DrawableGameComponent for this pattern.
**Tracked in:** not planned (refactored, functionally equivalent)

## Status text reformatted
**XNA behaviour:** Multi-line DrawString with exact XNA formatting showing effect name, free particle counts per system, and switch instructions.
**CNA port behaviour:** Three separate DrawString calls with slightly condensed layout. Content is equivalent.
**Root cause:** Minor formatting difference, no functional impact.
**Tracked in:** not planned
