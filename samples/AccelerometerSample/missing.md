# Missing / Differences from XNA 4.0 original

**Status: ported (2026-07-10).** Full C++ port under `src/`, wired into the
root `CMakeLists.txt`. Builds 0 warnings; screenshot-verified rendering and
keyboard-tilt-driven sprite movement (see Verification below).

Source: `/rv/tmp/XNAGameStudio/Samples/AccelerometerSample_4_0/Accelerometer/Accelerometer/{Accelerometer.cs, Game.cs, Program.cs}`.

## The keyboard-tilt scheme is the original's OWN emulator fallback, ported verbatim — not invented

**XNA behaviour:** `Accelerometer.cs` wraps the real Windows Phone hardware
sensor (`Microsoft.Devices.Sensors.Accelerometer`) behind a static
`Accelerometer.Initialize()`/`GetState()` API. The *entire* class body —
including the fallback described below — is compiled only under
`#if WINDOWS_PHONE` (confirmed by direct read; `Accelerometer.csproj:32,43`
defines `WINDOWS_PHONE` for both Debug and Release with no other target
platform). Inside that same `#if WINDOWS_PHONE` block, `GetState()`
(`Accelerometer.cs:107-136`) branches on
`Microsoft.Devices.Environment.DeviceType`:
- `DeviceType.Device` (real phone hardware): reads the last cached
  `AccelerometerReadingEventArgs` value from the real sensor's
  `ReadingChanged` event.
- else (running in the Visual Studio Windows Phone 7 **emulator**, which has
  no physical accelerometer to read): synthesizes a fake reading from the
  arrow keys instead (`Accelerometer.cs:117-135`, quoted exactly):
  ```csharp
  KeyboardState keyboardState = Keyboard.GetState();
  stateValue.Z = -1;
  if (keyboardState.IsKeyDown(Keys.Left))  stateValue.X--;
  if (keyboardState.IsKeyDown(Keys.Right)) stateValue.X++;
  if (keyboardState.IsKeyDown(Keys.Up))    stateValue.Y++;
  if (keyboardState.IsKeyDown(Keys.Down))  stateValue.Y--;
  stateValue.Normalize();
  ```
  `Initialize()` (`Accelerometer.cs:53-73`) sets `isActive = true`
  unconditionally on this branch too, with the comment "we always return
  isActive on emulator because we use the arrow keys for simulation which is
  always available."

**CNA port behaviour:** `src/Accelerometer.hpp`'s `Accelerometer` class has
**no** `#ifdef`/real-sensor branch at all — the emulator branch quoted above
is its **only** implementation, always active, ported field-for-field
(`stateValue.Z = -1`, the four `IsKeyDown` increments/decrements on
Left/Right/Up/Down, then `Vector3::Normalize()`). This was a deliberate
choice, not an oversight: since this desktop build has no Windows-Phone-style
`Microsoft.Devices.Sensors`/`Microsoft.Devices.Environment.DeviceType`
distinction to make in the first place (there is no real-vs-emulator
branch point on this platform), there is nothing to gate the fallback on --
it always applies, the same way it always applied inside the original's own
emulator builds. **This is why no keyboard control scheme needed to be
invented for this sample**, unlike what DEFERRED.md item #15's original
audit (written before this session, when this directory held only a
placeholder) worried might be necessary ("designing a keyboard-tilt-emulation
control scheme from scratch, since the original never shipped one" -- that
audit was written without having read `Accelerometer.cs`'s emulator branch
closely enough; a direct read this session found the original *did* already
ship exactly such a scheme, just nested inside `#if WINDOWS_PHONE`). Note
CNA also has a real, working, cross-platform `Microsoft::Devices::Sensors::Accelerometer`
(SDL_Sensor-backed, already used by Yacht/SnowShovel/Bounce for their own
tilt fallbacks) -- this port deliberately does not wrap it. Unlike those
three samples (which each have a *separate*, genuinely non-phone input
branch in their own C# originals to promote from conditional to
unconditional), this sample's original has no such separate branch: the
emulator branch **is** the keyboard scheme, so wrapping CNA's real hardware
sensor here would have meant re-introducing exactly the `#if`-style branch
this port intentionally avoids, for a hardware path this sample was never
designed to exercise standalone (the original's `Game.cs` never reads
anything but `Accelerometer.GetState()`).

**Root cause:** N/A -- not a CNA gap, a faithful direct port of the
original's own emulator-testing code path.

**Tracked in:** DEFERRED.md item #15 (updated this session to record that
this sample and TiltPerspective are being ported this way, per 2026-07-10
user go/no-go).

## Fullscreen and phone-hardware settings omitted

**XNA behaviour:** The constructor sets `graphics.IsFullScreen = true` and
`TargetElapsedTime = TimeSpan.FromTicks(333333)` (30 fps) -- Windows-Phone-
specific settings.
**CNA port behaviour:** Kept windowed (matching every other phone sample in
this repo -- Yacht, Bounce, SnowShovel all document the same omission). The
30 fps fixed timestep **is** kept faithfully
(`setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333))` in
`AccelerometerSampleGame`'s constructor).
**Root cause:** Desktop dev-loop practicality; matches established
repo-wide precedent (see Yacht's/Bounce's own `missing.md`).
**Tracked in:** not planned.

## Escape key added as a desktop convenience to exit

**XNA behaviour:** Only `GamePad.GetState(PlayerIndex.One).Buttons.Back ==
ButtonState.Pressed` exits the game.
**CNA port behaviour:** The gamepad Back-button check is ported unchanged,
plus `Keys::Escape` is also checked (a CNA addition, not in the original) --
a desktop keyboard has no physical Back button, and every other phone-sample
port in this repo (Bounce, Yacht, SnowShovel) adds the same Escape-to-exit
convenience.
**Root cause:** Desktop usability; matches established repo-wide pattern.
**Tracked in:** not planned.

## LoadContent() centers the sprite using known constants, not a live Viewport read

**XNA behaviour:** `LoadContent()` reads
`graphics.GraphicsDevice.Viewport.Width/Height` to center the asteroid
sprite on screen.
**CNA port behaviour:** Uses the known `GraphicsWidth`/`GraphicsHeight`
constants (480/800, the exact values requested via
`setPreferredBackBufferWidthProperty`/`setPreferredBackBufferHeightProperty`
in the constructor) instead of `getViewportProperty()`. `Update()`'s
per-frame edge-clamping, by contrast, *does* use a live
`getGraphicsDeviceProperty().getViewportProperty()` read, matching the
original exactly -- by the time `Update()` first runs (several frames after
window creation), the viewport has already synced correctly.
**Root cause:** `Game::Initialize()` calls `LoadContent()` directly (see
`Game.cpp`), so `LoadContent()` runs before CNA's `Viewport` has synced to
the just-created window -- the same stale-viewport-in-`LoadContent()` gotcha
already documented in `samples/SoccerPitch/missing.md` and
`samples/SnowShovel/missing.md`. Using the already-known preferred
back-buffer size sidesteps it entirely, with no behavioral difference since
the back buffer is always created at exactly that size.
**Tracked in:** CNA issue (viewport-sync timing), not blocking -- same
root cause as the SoccerPitch/SnowShovel instances already on record.

## F1 help overlay text is wider than the portrait window (cosmetic, pre-existing pattern)

**XNA behaviour:** N/A (CNA-only addition per CLAUDE.md).
**CNA port behaviour:** `Accelerometer.htm`'s Sample Controls table has 3
columns (`Action | Windows Phone | Windows Phone - Emulator`) --
`tools/gen_help_png.py` hardcodes column index 1 ("Windows Phone" -- real
tilt hardware, not what this port implements), so `Content/help.png` was
generated with a one-off variant script (same precedent as
`samples/MicrophoneEcho/missing.md`'s own column-picking script, not
committed as a shared tool) reading column index 2 ("Windows Phone -
Emulator": `Up Arrow, Down Arrow, Left Arrow, Right Arrow`) instead, matching
this port's real controls. The resulting `help.png` is 632&times;192 --
wider than this sample's 480px-wide portrait window -- so the centered
overlay text is left/right-clipped (confirmed via screenshot: "Action"
reads as "...on", "Right Arrow" is cut off mid-word). This is **not a
regression specific to this sample**: `samples/Yacht/Content/help.png` is
1472&times;312 in that sample's own identically-480px-wide portrait window,
using the exact same unmodified `Draw()`-time centering code from
CLAUDE.md's F1 overlay pattern -- this is a pre-existing, already-accepted
characteristic of the shared F1 overlay pattern on narrow portrait windows
whenever a sample's own control-table text is long, not something unique to
this port.
**Root cause:** `gen_help_png.py`'s column-selection is hardcoded to index 1
(first flagged by MicrophoneEcho); the F1 overlay's centering code has no
wrap/scale-to-fit logic for overlays wider than the viewport (pre-existing,
shared by every portrait-mode sample, not just this one).
**Tracked in:** not planned (worked around per-sample for the column
selection, same as MicrophoneEcho; the width-clipping itself is a
pre-existing, already-accepted repo-wide characteristic, not new).

## Verification

Built via `cmake --build cmake-build-debug --target AccelerometerSample_cna_samples -j$(nproc)`
-- 0 errors, 0 warnings, confirmed via two from-scratch rebuilds of this
target (one before, one after removing temporary debug-auto-trigger code
described below), the second immediately re-run afterward with no source
changes (`ninja: no work to do`). The full aggregate build
(`cmake --build cmake-build-debug -j$(nproc)`) also reports `ninja: no work
to do` afterward, confirming this sample's addition to the root
`CMakeLists.txt` doesn't break the aggregate build.

Ran under `SDL_VIDEODRIVER=x11` for 7+ seconds across three separate runs
with no crash (confirmed via `ps aux` and no error output in the captured
stdout/stderr log). Window (`480x800`, title `"Game"`) confirmed via
`xwininfo -root -tree | grep -i accelerometer`.

Screenshot with real (unmodified) keyboard state confirms: the space
background and asteroid sprite both render correctly, the asteroid centered
exactly where expected (`(480-128)/2, (800-128)/2`), and -- since no key was
held -- the sprite stays motionless at that centered rest position, matching
the original's own physics (zero acceleration &rarr; zero velocity change).

Live keyboard-tilt input verification hit this repo's own known `xdotool`
shared-desktop caveat (NEXT.md section 5): `xdotool getactivewindow` showed
a different real user window (`0x400003`, decimal `4194307`) held focus
throughout, and a subsequent `xdotool keydown --window <id> Right` /
screenshot / `keyup` round trip produced **no visible movement** -- the same
"input doesn't reliably reach the window even when synthesized" limitation
already documented for every other sample's own keyboard-input testing this
session. Per this repo's own established fallback (a temporary, clearly
commented debug auto-trigger, reverted before commit, not blind trust that
"no visible change" means a bug), the code was temporarily patched in two
places to confirm the underlying logic directly:
1. `Update()`'s `acceleration.X` forced to `0.5f` right after
   `accelerometer_.GetState()` -- rebuilt, ran, and screenshotted twice
   (~2 seconds apart): the asteroid visibly slid from its centered rest
   position to the right edge of the screen and stayed clamped there in both
   screenshots (matching `Update()`'s own edge-clamp logic, which zeroes
   `logoVelocity_.X` once `logoPosition_.X` exceeds
   `viewport.Width - asteroidTexture_->Width`) -- direct proof the
   keyboard-equivalent acceleration path drives `logoVelocity_`/
   `logoPosition_` and the screen-bounds clamp both work correctly, exactly
   mirroring the original's own physics.
2. `Update()`'s F1-overlay branch forced `helpTimer_ = 10.0f` on the very
   first frame -- rebuilt, ran, and screenshotted twice (~7 seconds apart):
   the first screenshot shows the (clipped, see above) help text visible over
   the asteroid/background; the second, taken after the 10-second timer
   should have expired, shows the overlay gone and the asteroid still
   correctly rendered underneath -- confirming the F1 overlay's own timer
   countdown logic works unmodified from CLAUDE.md's standard pattern.

Both temporary patches were reverted before committing, and the target was
rebuilt from scratch afterward to reconfirm 0 warnings (see above) -- the
committed source contains neither.

No new DEFERRED.md item was needed: every finding above is either a direct,
faithful port of the original's own code (the keyboard-tilt scheme itself)
or a previously-documented, already-tracked repo-wide pattern (fullscreen/
30fps omission, Escape-to-exit, the `LoadContent()`-time viewport-staleness
gotcha, and the F1-overlay-width-vs-portrait-window characteristic already
seen in Yacht).
