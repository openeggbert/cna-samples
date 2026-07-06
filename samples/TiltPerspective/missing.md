# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `TiltPerspectiveSample.htm`) so the blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the scope decision below
is made.

Source: `/rv/tmp/XNAGameStudio/Samples/TiltPerspective_4_0/TiltPerspective/AccelerometerHelper.cs`
and `TiltPerspective/TiltPerspectiveSample.cs`.

## Blocker: needs a scope decision, not a CNA gap

**XNA behaviour:** A perspective-shifted 3D scene (and a ball-in-a-bowl physics demo,
`BallSimulation.cs`) is controlled purely by tilting the phone. `AccelerometerHelper.cs`
wraps `Microsoft.Devices.Sensors.Accelerometer` in a `GameComponent`
(`IAccelerometerService`) that exposes `RawAcceleration`/`SmoothAcceleration`. Unlike
AccelerometerSample, this file has **no** `#if WINDOWS_PHONE` split at all — it targets
Windows Phone unconditionally (confirmed by the project file, `TiltPerspective.csproj:32,43`,
which only defines a `Debug|Windows Phone`/`Release|Windows Phone` configuration, and by
`using Microsoft.Devices.Sensors;` at the top of `AccelerometerHelper.cs` with no
alternate branch).

When a physical sensor isn't available, the helper does **not** fall back to any
keyboard/gamepad input at all — it synthesizes a canned, non-interactive wobble instead:
"If the default accelerometer is not available (for example, when running on the
emulator), fake readings will be synthesized so you can still see some motion"
(`AccelerometerHelper.cs:37-38`), implemented as an automatic sinusoidal tilt driven by
`FakeRollPhi`/`FakeRollTheta`/`FakeRollSpeed` fields (`AccelerometerHelper.cs:70-74`) with
no reference to `Keyboard`/`GamePad` anywhere in the file. There is strictly less
non-phone control surface here than in AccelerometerSample — not even an emulator
key-mapping to adapt.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** This is **not** a missing CNA feature. CNA's
`Microsoft::Devices::Sensors::Accelerometer` (`cna/src/Microsoft/Devices/Sensors/Accelerometer.cpp`)
is a real, working, cross-platform implementation backed by `SDL_Sensor`, already
proven working — with a keyboard/gamepad fallback for the no-sensor desktop case — in
the already-ported Yacht, SnowShovel, and Bounce samples. Porting *this* sample
specifically means **designing a keyboard-tilt-emulation control scheme entirely from
scratch**, since the original never shipped an interactive fallback of any kind (only
an automatic non-interactive wobble): a real product/scope decision for the repo
maintainer to make (what keys drive perspective tilt vs. ball-in-bowl gravity, whether
to keep the automatic wobble as a no-input default, etc.), not an engine capability
gap to fix.

**Tracked in:** DEFERRED.md item #15
