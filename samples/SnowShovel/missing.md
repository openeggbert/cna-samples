# Missing / Differences from XNA 4.0 original

## Added: real accelerometer, gamepad/touch/keyboard fallback unchanged
**XNA behaviour:** The original only reads the accelerometer inside an
`#if WINDOWS_PHONE` block (via a `ReadingChanged` event handler storing the latest
`AccelerometerReading`), which overrides `accelX`/`accelY` every frame. On a desktop
Windows build (`WINDOWS_PHONE` undefined) that whole block compiles out, leaving
gamepad-thumbstick + touch + arrow-key input as the only way to move the shovel.
**CNA port behaviour:** CNA's `Microsoft::Devices::Sensors::Accelerometer` is a real
SDL_Sensor-backed sensor, not phone-only, so the `#if` is replaced with a runtime check
(`Accelerometer::Initialize()`/`GetTilt()` in `Accelerometer.hpp`, following the same
pattern as Bounce and Yacht): when real hardware is present, tilt overrides
gamepad/touch/keyboard every frame, exactly like the phone branch did; when absent (the
normal desktop case), `GetTilt()` returns `std::nullopt` and the original's own
gamepad/touch/keyboard handling (already present, unconditionally, in its desktop code
path) runs unmodified.
**Root cause:** No accelerometer hardware on the development machine; this is a hardware
gate, not a CNA bug.
**Tracked in:** Not planned — deliberate, documented addition, matching the precedent set
by Bounce and Yacht.

## Added: mouse fallback for moving the shovel and for tap-to-start/restart
**XNA behaviour:** The original never reads `Mouse` at all — only `TouchPanel`,
`GamePad`, and `Keyboard` (arrow keys). "Move Shovel" and "Start/Restart" are
Touch-Screen-or-Tilt on the phone, Arrow-Keys-or-Spacebar on desktop Windows; there was
never a mouse-driven control scheme in the original, on any platform.
**CNA port behaviour:** since CNA does not synthesize `TouchPanel` events from mouse
input (only real SDL finger events feed it), a mouse-driven fallback was added so the
sample is fully playable on a mouse-only desktop, following the same
per-sample-addition precedent as GesturesSample/TouchThumbsticks: holding the left mouse
button and dragging moves the shovel toward the cursor (same relative-offset calculation
as the real touch path, just gated on `ButtonState::Pressed` instead of an active touch,
mirroring "touch requires contact"); a left click starts the game from the pre-game
screen and, on its rising edge only (`MouseLeftJustPressed()`, mirroring
`TouchLocationState::Pressed`), restarts from the game-over screen.
**Root cause:** No touchscreen on the development machine, and the original's own
keyboard-only fallback (Arrow Keys / Spacebar) is not the control scheme most desktop
users instinctively reach for on a "tap the screen" game.
**Tracked in:** Not planned — deliberate, documented addition, matching the precedent set
by GesturesSample and TouchThumbsticks.

## Adapted: world-to-screen matrix computed from known constants, not GraphicsDevice.Viewport
**XNA behaviour:** `Initialize()` computes `worldToScreenMatrix` from
`GraphicsDevice.Viewport.Width`/`Height`, which is safe on the phone — there is no
windowing system to race against, so the viewport already reflects the real screen size by
the time `Initialize()` runs.
**CNA port behaviour:** Querying `getGraphicsDeviceProperty().getViewportProperty()` inside
`Initialize()` returned a stale/wrong size (`1333x800` instead of the requested `480x800`)
even though `Game::DoInitialize()` calls `GraphicsDeviceManager::CreateDevice()` before
`Initialize()`, and the actual SDL window was already correctly sized at `480x800` (confirmed
via `xwininfo`). The mismatch produced a ~4.9x-too-large scale factor instead of ~1.77x,
pushing all text and the shovel sprite off the right/bottom edge of the window. Worked
around by using the `GraphicsWidth`/`GraphicsHeight` constants passed to
`setPreferredBackBufferWidthProperty`/`HeightProperty` in the constructor directly, instead
of re-querying the viewport. `Yacht`/`GameStateManagement`/other samples that call
`getViewportProperty()` only do so later, from `Update()`/`Draw()` (see e.g. Yacht's
`ScreenManager::SafeArea()`), never from `Initialize()`, so they don't hit this.
**Root cause:** CNA's `Viewport`/`PresentationParameters` back-buffer size apparently isn't
synced to the just-created SDL window until at least one event-pump cycle has run (i.e. not
yet valid the moment `Initialize()` is entered). Not root-caused further inside CNA itself —
the sample-level workaround is exact (we already know the real requested size) and low-risk,
so this wasn't escalated to a CNA framework fix this time.
**Tracked in:** Not fixed in CNA; worked around in this sample. Worth a NEXT.md follow-up if
a future sample needs `Viewport` sizing genuinely inside `Initialize()`.

## Adapted: manual "MM:SS.T" time formatting instead of String::Format custom picture format
**XNA behaviour:** `string.Format("Time: {0:00}:{1:00.0}", timeRemaining.Minutes, ...)` uses
.NET custom numeric picture formats (`"00"`, `"00.0"`) for zero-padded minutes/seconds.
**CNA port behaviour:** sharp-runtime's `System::String::Format` only implements the
standard .NET specifiers (`D`, `F`, `G`, `E`, `X`) — a leading-`0` custom picture format like
`"00"` falls through to a plain, unpadded conversion. Worked around with a small
`FormatMinutesSeconds()` helper (manual zero-padding + `std::setprecision(1)`), the same
approach `Platformer`'s `pad2` helper already uses for its own countdown timer.
**Root cause:** `String::Format` custom picture-format support is a larger feature than this
one sample needs; not escalated to a CNA fix.
**Tracked in:** Not planned — same class of adaptation as `Platformer`'s `pad2`.

## RNG: one shared stream (and a different draw order) instead of two independent streams
**XNA behaviour:** `Game.cs` declares two entirely independent `Random`
instances: the private nested `Snowflake` class has its own `static Random
rand = new Random();` (`Game.cs:34`), used only inside `Snowflake`'s
constructor in the exact order `Scale`, then `AngularVelocity`, then
`Tint.R`/`Tint.G`/`Tint.B` (`Game.cs:52-58`); the outer `Game` class has a
separate `Random rand;` field (`Game.cs:128`), used only by `Snow()` to pick
spawn positions/velocities/sprite index.
**CNA port behaviour:** `SnowShovelGame` has a single `System::Random rand_;`
member, forwarded by reference into every `Snowflake` constructor call
(`Snowflake.hpp:27`) as well as used directly by `Snow()` -- one shared
stream instead of two independent ones. Worse, `Snowflake`'s `Tint` is built
in the C++ member-initializer list (3 `rand.NextDouble()` calls), which the
language guarantees runs *before* the constructor body (`Scale`/
`AngularVelocity`), the reverse draw order from the original; argument
evaluation order within the 3-arg `Tint(...)` call and within `Snow()`'s
multi-arg `emplace_back(...)` call is also unspecified by the C++ standard,
unlike C#'s guaranteed left-to-right evaluation.
**Root cause:** Simplification during porting -- one member `Random` was used
everywhere instead of replicating the original's two-instance split.
`System::Random`'s default constructor seeds from `std::random_device`
(non-deterministic, like .NET's own default `Random()`), so neither version
is bit-reproducible run-to-run regardless -- this is a structural RNG-usage
deviation (shared vs. independent streams, different consumption order), not
a reproducibility regression. No visible gameplay bug: scale/spin/tint still
land in the same original ranges, just drawn from a differently-ordered/
shared sequence.
**Tracked in:** Not planned -- no visible behavioral difference (snowflake
appearance is still "randomly varied" as intended), just a documented
structural deviation.

## Font substitution: Segoe UI Mono / Moire -> DejaVu Sans Mono / DejaVu Sans
**XNA behaviour:** `TitleFont.spritefont` specifies "Segoe UI Mono", Regular,
14pt. `ScoreFont.spritefont` specifies "Moire", Regular, 12pt — an unusual
decorative/display font, not a standard Windows system font.
**CNA port behaviour:** Generated from DejaVu Sans Mono (`TitleFont`) and
DejaVu Sans (`ScoreFont`) at the same point sizes via `tools/make_font.py`
(CNA has no `.spritefont`/TTF-at-runtime pipeline). Glyph metrics, and for
`ScoreFont` the overall letterform style, differ noticeably from "Moire",
which is not an open font.
**Root cause:** XNA `.xnb` SpriteFont binaries are not supported; neither
Segoe UI Mono nor Moire is available as an open TTF, so DejaVu substitutes
are used per this project's established convention (see CLAUDE.md's Assets
section).
**Tracked in:** Not planned — same class of adaptation as this project's
other font substitutions (see e.g. `samples/Yacht/missing.md`).

## Verification note
Interactively verified via `xdotool` + screenshots: pre-game screen (title, centered
instructions, score/time/elapsed-time HUD, shovel, falling snowflakes all correctly
positioned after the viewport-timing fix above), SPACE starting the game from the pre-game
screen, score increasing as snowflakes are caught, the countdown timer turning progressively
more red, automatic transition to the "GAME OVER" screen when the timer runs out, and the F1
help overlay. The symmetric SPACE-to-restart transition from "GAME OVER" back to the
pre-game screen was not independently re-confirmed by `xdotool` screenshot in the same
session — mid-session, synthetic keyboard/mouse input intermittently stopped reaching the
sample's window because the desktop session had other windows actively in use (see the
`xdotool`/shared-desktop gotcha in NEXT.md section 5), and further synthetic input was
intentionally stopped rather than risk sending keystrokes to the wrong window. The user then
manually tested the mouse fallback themselves on real hardware and confirmed it works
("funguje" — works) after the mouse click-and-drag/click-to-start/click-to-restart fallback
(see the section above) was added; the keyboard/gamepad/touch paths (including the restart
transition) were not separately called out in that manual pass but share the exact same
code path already verified above for the analogous start transition.

**Not yet verified on real accelerometer hardware:** everything confirmed above exercises
the keyboard/gamepad/mouse/touch *fallback* paths only. The actual tilt-sensor code path
(`Accelerometer::GetTilt()` returning real `SDL_Sensor` data, see the "Added: real
accelerometer" section above) has never been run on a device with physical accelerometer
hardware — this development machine has none, so `Accelerometer::getIsSupportedProperty()`
always returns false here and the fallback path is all that's ever executed. Tracked in
NEXT.md section 5 ("real hardware accelerometer shake/tilt (Yacht, SnowShovel) has never
been tested on a device with a physical sensor").

## No known differences beyond the above
Snowflake spawning/bouncing physics, wave escalation (more snowflakes, shorter bonus time
down to a 500ms floor), the shovel hit-test and scoring, the three game states
(pre-game/game/post-game) and their transitions, and the HUD/instruction text layout are a
direct, faithful port of `Game.cs`. Screen resolution (480x800 portrait, matching the
original's Zune-HD-native 272x480 world scaled up to the Windows Phone default) and the
drop-shadow text style match the original.
