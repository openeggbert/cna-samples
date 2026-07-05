# Missing / Differences from XNA 4.0 original

## XML settings hand-translated to C++ construction code
**XNA behaviour:** `Content/*.xml` (EmitterSettings, ExplosionSettings,
ExplosionSmokeSettings, SmokePlumeSettings) are parsed at content-build time
by the stock XNA `IntermediateSerializer` into `ParticlesSettings.ParticleSystemSettings`
instances, then loaded via `Content.Load<ParticleSystemSettings>(name)`.
**CNA port behaviour:** `ParticleSystem.hpp`'s `BuildExplosionSettings()`,
`BuildExplosionSmokeSettings()`, `BuildSmokePlumeSettings()`, and
`BuildEmitterSettings()` hand-translate the exact values from each XML file
into C++ construction code, called directly instead of going through
`ContentManager::Load`.
**Root cause:** CNA has no general content-pipeline deserializer for custom
types.
**Tracked in:** Same precedent as DynamicMenu's `MenuPage2.xml` and
NinjAcademy's `Animations.xml`/`Configuration.xml` -- only 4 small files
here, well within the range this hand-translation approach scales to
(unlike RolePlayingGame's 281 files, which needed a real runtime loader
instead).

## `DrawableGameComponent`/`Game.Components` not used
**XNA behaviour:** Each `ParticleSystem` inherits `DrawableGameComponent`
and is registered in `Game.Components`; the base `Game.Update()`/`Draw()`
dispatch to all four systems automatically, in `DrawOrder` order.
**CNA port behaviour:** The four `ParticleSystem` instances are plain
`std::optional<ParticleSystem>` members of `ParticleSampleGame`, updated and
drawn with explicit calls in a fixed order (alpha-blended systems, then the
additive explosion system last) that reproduces the original's DrawOrder-
sorted component order exactly.
**Root cause:** Design choice, not a CNA limitation -- CNA's
`GameComponentCollection` works fine elsewhere in this repo (e.g.
HoneycombRush). Matches the precedent already set by the sibling
`samples/ParticleSample` port: this sample's four particle systems need no
automatic per-frame dispatch beyond what the game's own `Update()`/`Draw()`
already has to do explicitly anyway (drawing message text before the
particle systems, and controlling additive-vs-alpha layering), so avoiding
`GameComponent` ownership complexity for no behavioural difference.
**Tracked in:** Same class of adaptation as `samples/ParticleSample/missing.md`.

## Touch tap and Xbox thumbstick input
**XNA behaviour:** `#if XBOX` uses the left thumbstick to move the emitter;
otherwise (Windows and Windows Phone) the `Mouse` positions the emitter.
`TouchPanel.EnabledGestures`/`ReadGesture` (unconditional, all platforms)
supplies an additional "tap to switch effect" input alongside Space/A.
**CNA port behaviour:** Ported the non-Xbox branch as-is: `Mouse::GetState()`
positions the emitter, and `TouchPanel::ReadGesture()` is polled for a Tap
gesture exactly like the original. On this desktop (no touchscreen),
`TouchPanel` never produces a gesture -- the same behaviour real Windows
without a touch digitizer already has in the shipped original, so this
required no invented fallback of any kind.
**Root cause:** N/A -- faithful port of the original's own desktop code path.
**Tracked in:** N/A.

## Verification: idle rendering confirmed; interactive switch not confirmed
**What was checked:** Built and ran `Particles2DPipeline_cna_samples` under
`SDL_VIDEODRIVER=x11`. Two idle screenshots (no synthetic input sent before
capture) confirm correct rendering of the default "Explosions" effect across
both halves of its cycle: the additive fiery flash, and the soft
alpha-blended smoke drifting afterward. Free-particle counters shown in the
on-screen text move correctly as particles are consumed and recycled
(including the "grow free-particle pool by 10" path, exercised naturally
during these runs with no crash).
**What was not confirmed:** Switching to the SmokePlume/Emitter states via
Space/mouse. Before attempting it, `xdotool getactivewindow` did not
resolve to this sample's own window (matching the `feedback_xdotool_shared_desktop`
memory gotcha -- input was not reliably reaching sample windows this
session), so no synthetic input was sent. A deliberate playthrough (switch
through all three states, confirm the mouse-driven emitter trail) is still
owed once input reliably reaches sample windows again.
