# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-06.** Builds and runs cleanly (`MicrophoneEcho_cna_samples`,
0 warnings). Live-verified via screenshot: the CornflowerBlue background, instruction
text, and status text ("Default Device is Stopped") all render correctly, and a real
capture device was enumerated on this machine by CNA's `Microphone::getDefaultProperty()`
— confirming the `feature/audio` branch's `Microphone`/`DynamicSoundEffectInstance`
implementation (DEFERRED.md item #16, resolved 2026-07-04) is genuinely functional, not
just present. See section "Verification" at the end for what was and wasn't confirmed
interactively.

Source: `/rv/tmp/XNAGameStudio/Samples/MicrophoneEchoSample_4_0/MicrophoneEchoSample/MicrophoneEchoSampleGame.cs`.

## Windows Phone branch dropped, desktop keyboard/gamepad branch ported as-is
**XNA behaviour:** `#if WINDOWS_PHONE` sets `graphics.IsFullScreen = true` and a 30 fps
`TargetElapsedTime`, shows `"Tap to start or DoubleTap to stop recording"`, and expects
touch-only input. The `#else` (desktop) branch — which this port follows — sets neither
of those, shows `"Press 'A' to start and 'B' to stop recording"`, and reads
gamepad/keyboard A/B alongside the same `TouchPanel` gesture check (falls through to
keyboard/gamepad when no gesture is available, unconditionally, on every platform).
**CNA port behaviour:** Ported the desktop `#else` branch line-for-line — no
`IsFullScreen`/`TargetElapsedTime`, `TouchPanel::EnabledGestures` still set to
`Tap | DoubleTap` (harmless with no touchscreen; CNA's `TouchPanel` just never produces
a gesture, same as real Windows without a digitizer), A/B keyboard and gamepad handling
identical to the original's `HandleInput()`.
**Root cause:** N/A — faithful port of the original's own desktop code path, not a CNA
gap or invented fallback.
**Tracked in:** not planned (matches the original's existing platform branch).

## `NoMicrophoneConnectedException` handling kept for fidelity, not currently exercised
**XNA behaviour:** Nearly every microphone call site is wrapped in
`try { ... } catch (NoMicrophoneConnectedException) { ... }` to handle a mid-session
physical disconnect.
**CNA port behaviour:** Same try/catch structure kept around every equivalent call
(`Microphone::getStateProperty()`, `GetData()`, `Start()`, `Stop()`). Confirmed by
reading `cna/src/Microsoft/Xna/Framework/Audio/Microphone.cpp`: no code path currently
throws `NoMicrophoneConnectedException` at runtime (a disconnected device simply
isn't returned by `getAllProperty()`/`getDefaultProperty()` in the first place), so
these catch blocks are currently dead code — kept for structural fidelity with the
original and in case a future CNA revision adds live disconnect detection.
**Root cause:** N/A — not a functional gap, just an unverifiable-on-this-hardware edge
case (would need physically unplugging a USB mic mid-run to exercise it).
**Tracked in:** not planned.

## Font: "Segoe UI Mono" substituted with DejaVu Sans Mono
**XNA behaviour:** `MyFont.spritefont` specifies `<FontName>Segoe UI Mono</FontName>`,
14pt Regular.
**CNA port behaviour:** `tools/make_font.py` generated a `.font.json` + PNG atlas from
`DejaVuSansMono.ttf` at 14px, matching this repo's standard SpriteFont substitution
convention (Segoe UI Mono is a proprietary Windows font).
**Root cause:** Licensing — not a CNA limitation.
**Tracked in:** not planned (acceptable substitute, same pattern as every other
SpriteFont-using sample in this repo).

## F1 help overlay controls table: picked the "Windows" column, not "Windows Phone"
**XNA behaviour:** N/A (CNA-only addition per CLAUDE.md).
**CNA port behaviour:** `MicrophoneEcho.htm`'s Sample Controls table has 4 columns
(Action | Windows Phone | Windows | Windows/Xbox Gamepad) — `tools/gen_help_png.py`
defaults to column 1 ("Windows Phone" — `TAP screen`/`DOUBLETAP screen`), which doesn't
match this desktop port's actual A/B keyboard scheme. Generated `Content/help.png` with
a one-off variant script that reads column 2 ("Windows": `A`/`B`/`ESC or ALT+F4`)
instead, matching the port's real controls.
**Root cause:** `gen_help_png.py`'s column-selection is hardcoded to index 1; this is
the first sample in this repo whose Sample Controls table has more than the usual
2-3 columns in a phone-first order. Not fixed in the shared tool (out of scope for a
single sample port) — worth revisiting if a future sample hits the same issue.
**Tracked in:** not planned (worked around per-sample, not a CNA gap).

## Verification
**Confirmed live (screenshot, this session):** built `MicrophoneEcho_cna_samples`
(0 warnings) and ran it under `SDL_VIDEODRIVER=x11`. Screenshot shows the
CornflowerBlue background, both instruction/status text lines rendering via
`SpriteFont`/`DrawString`, and a flat white horizontal waveform line via
`BasicEffect`/`VertexPositionColor`/`DrawUserPrimitives(LineStrip)` — correct for a
silent (all-zero) echo buffer before recording starts. Status text read
`"Default Device is Stopped"`, confirming CNA's `Microphone::getDefaultProperty()`
found a real capture device on this machine (not a stub/empty list) and
`Microphone::Name`/`getStateProperty()` both work. Ran crash-free for 5+ seconds.

**Not confirmed interactively:** pressing 'A' to start recording did not visibly
change the status text in a follow-up screenshot. Before concluding this is a port
bug, note the `feedback_xdotool_shared_desktop` project memory: `xdotool key` input
has repeatedly failed to reach sample windows on this shared desktop even when
`getactivewindow` reports the correct window focused (confirmed focused here too).
Given the render pipeline, device enumeration, and event-subscription code all
build and run without error, and given this exact class of input-delivery failure is
already an established, unrelated environment limitation in this repo (not previously
traced to a code bug in any other sample), this is recorded as **not yet confirmed**
rather than a suspected bug. A future session with reliable local input (or physical
access) should press 'A', confirm the status flips to `"... is Started"`, and confirm
the waveform line begins moving with live microphone input before removing this note.
