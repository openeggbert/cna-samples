# Missing / Differences from XNA 4.0 original

## Additive blending for explosion particles not used
**XNA behaviour:** `ExplosionParticleSystem` uses `BlendState.Additive` giving the explosion a bright glowing look. `ExplosionSmokeParticleSystem` and `SmokePlumeParticleSystem` use `BlendState.AlphaBlend`. Each particle system draws in its own `SpriteBatch.Begin(SpriteSortMode.Deferred, blendState)`/`End()` pair (`ParticleSystem.Draw` in `ParticleSystem.cs`).
**CNA port behaviour:** All three particle systems are drawn in one shared `SpriteBatch::Begin()/End()` block with default alpha blend. The explosion still fades in/out via alpha but lacks the additive glow.
**Root cause:** CNA's Vulkan backend has a bug where only the last `SpriteBatch::Begin()/End()` pair per frame is rendered, so porting the original's three-Begin/End structure verbatim would make two of the three effects invisible on that backend. Merging all three draws into one block with default blend sidesteps this unconditionally. Note this is stricter than the project's current default backend actually requires: `CMakeLists.txt` defaults `CNA_GRAPHICS_BACKEND` to `EASYGL`, and multiple Begin/End pairs per frame do compose correctly there (confirmed by AimingSample's and GameStateManagement's `missing.md`, which keep separate Begin/End pairs and only call out Vulkan as broken). A faithful three-Begin/End port with `BlendState.Additive` for the explosion would render correctly on EasyGL — only Vulkan would break — but the single-block workaround was kept here for Vulkan-backend safety.
**Tracked in:** CNA issue (Vulkan multi-batch bug)

## SpriteFont substituted
**XNA behaviour:** `font.spritefont` (built via the XNA Content Pipeline's `FontDescriptionImporter`/`FontDescriptionProcessor`) specifies **"Segoe UI"**, 14 pt, 2 px extra spacing, Regular style, ASCII 32-126.
**CNA port behaviour:** `Content/font.font.json` + `Content/font.png` generated from **DejaVu Sans** at 14 px via `tools/make_font.py` (line spacing 17 px, matching DejaVu Sans's ascent+descent at that size). Glyph metrics differ slightly from Segoe UI, so text widths are not pixel-identical to the original.
**Root cause:** CNA has no XNA Content Pipeline / `.spritefont` support (DEFERRED.md item 1), and Segoe UI is a proprietary Microsoft font not available on this platform.
**Tracked in:** not planned (routine asset substitution, same pattern as GameStateManagement/CatapultWars)

## DrawableGameComponent not used
**XNA behaviour:** Each `ParticleSystem` inherits from `DrawableGameComponent` and is registered in `Game.Components`. Update/Draw are called automatically via the component system.
**CNA port behaviour:** Particle systems are managed directly as `std::unique_ptr` members of the Game class, with Update/Draw called explicitly.
**Root cause:** CNA component system not confirmed to support DrawableGameComponent for this pattern.
**Tracked in:** not planned (refactored, functionally equivalent)

## Touch tap gesture not implemented
**XNA behaviour:** Constructor sets `TouchPanel.EnabledGestures = GestureType.Tap;`. In
`HandleInput`, all pending gestures are drained each frame and a `GestureType.Tap` also
triggers switching between the Explosions and SmokePlume effects (in addition to Space
and gamepad A).
**CNA port behaviour:** Only keyboard Space and gamepad A switch effects; there is no
touch/tap handling.
**Root cause:** Desktop port targets keyboard/gamepad; touch is phone/tablet-specific
and CNA's touch support was not wired up for this sample.
**Tracked in:** not planned

## Status text reformatted
**XNA behaviour:** Multi-line DrawString with exact XNA formatting showing effect name, free particle counts per system, and switch instructions (instructions text also mentions tapping the screen).
**CNA port behaviour:** Three separate DrawString calls with slightly condensed layout and wording (no mention of tapping, since touch isn't implemented). Numeric content (effect name, free particle counts) is equivalent.
**Root cause:** Minor formatting/wording difference, no functional impact.
**Tracked in:** not planned

## Windows Phone full-screen/frame-rate branch removed
**XNA behaviour:** `#if WINDOWS_PHONE` branch in the constructor sets
`graphics.IsFullScreen = true` and `TargetElapsedTime = TimeSpan.FromTicks(333333)`
(30 fps) for the phone build.
**CNA port behaviour:** No phone branch exists; the game runs windowed at the default
frame rate.
**Root cause:** Desktop-only port target; phone-specific XNA platform code is out of
scope.
**Tracked in:** not planned
