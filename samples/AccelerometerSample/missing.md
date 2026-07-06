# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `Accelerometer.htm`) so the blocker is documented where a future porting
session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the scope decision below
is made.

Source: `/rv/tmp/XNAGameStudio/Samples/AccelerometerSample_4_0/Accelerometer/Accelerometer/Accelerometer.cs`
and `Accelerometer/Accelerometer/Game.cs`.

## Blocker: needs a scope decision, not a CNA gap

**XNA behaviour:** A sprite is moved around the screen purely by tilting the phone.
`Accelerometer.cs` wraps `Microsoft.Devices.Sensors.Accelerometer` behind a static
`Accelerometer.Initialize()`/`GetState()` API, but the entire implementation —
including the one piece of non-hardware input handling that exists — is compiled
only under `#if WINDOWS_PHONE` (confirmed by direct read of `Accelerometer.cs`
and by the project file, `Accelerometer.csproj:32,43`, which defines
`WINDOWS_PHONE` for both Debug and Release and has no other target platform). The
sample's `.csproj` targets exactly one platform: Windows Phone.

The one fallback that does exist — arrow-key emulation inside `GetState()`
(`Accelerometer.cs:118-134`, guarded by `Microsoft.Devices.Environment.DeviceType ==
DeviceType.Device` vs. the WP7 *emulator*) — is itself nested entirely inside the
`#if WINDOWS_PHONE` block alongside phone-SDK-only types
(`Microsoft.Devices.Sensors.Accelerometer`, `Microsoft.Devices.Environment`), so it
cannot simply be un-`#if`'d the way Yacht/SnowShovel/Bounce/Orientation's existing
desktop branches were: there is no non-phone-compiled branch anywhere in this file
to promote to the default path.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** This is **not** a missing CNA feature. CNA's
`Microsoft::Devices::Sensors::Accelerometer` (`cna/src/Microsoft/Devices/Sensors/Accelerometer.cpp`)
is a real, working, cross-platform implementation backed by `SDL_Sensor`
(`getIsSupportedProperty()` gates on Desktop/Android/iOS and does a genuine hardware
probe), already proven working — with a keyboard/gamepad fallback for the no-sensor
desktop case — in the already-ported Yacht, SnowShovel, and Bounce samples. Porting
*this* sample specifically means **designing a keyboard-tilt-emulation control
scheme from scratch**, since the original never shipped one: a real product/scope
decision for the repo maintainer to make (which keys map to which tilt axes, whether
to reuse the emulator's arrow-key mapping as a starting point, etc.), not an engine
capability gap to fix.

**Tracked in:** DEFERRED.md item #15
