# Missing / Differences from XNA 4.0 original

## Scenario #4 enabled instead of the shipped inert default
**XNA behaviour:** `OrientationSample.cs` ships four alternative orientation-
handling configurations as one live default (Scenario #1: full resolution,
locked to landscape, `enableOrientationLocking = false`) plus three fully-
written, commented-out alternates in the same file. The sample's own `.htm`
documentation states: "In order to see all four approaches, change the
sample's code as instructed" -- this was authored as a code tutorial meant
to be edited and recompiled, not a single interactive demo out of the box.
**CNA port behaviour:** Scenario #4 (full resolution, both landscape and
portrait supported, dynamic tap-to-lock/unlock the current orientation) is
enabled instead of the literal shipped default. This is real, complete code
already present in the original file -- nothing was invented -- but it is a
deliberate choice of which documented alternate to port, since the literal
default renders one static image with no interactivity at all. Same spirit
as NinjAcademy's real `NameEntryScreen` instead of a `Guide` stub.
**Root cause:** N/A -- editorial choice among the original's own shipped
alternates, made because scenario #1 alone is not demonstrable.
**Tracked in:** NEXT.md (investigated 2026-07-05, ported same session).

## `LayoutSample.cs` not ported (dead code)
**XNA behaviour:** `LayoutSample.cs` exists in the same source directory.
**CNA port behaviour:** Not ported. Confirmed dead: `Program.cs` only ever
constructs `OrientationSample`, and `LayoutSample.cs` is not even listed in
`OrientationSample (Phone).csproj`'s `<Compile Include>` items -- it was
never part of the compiled original.
**Root cause:** N/A -- avoids porting genuinely unreachable source.
**Tracked in:** N/A.

## Keyboard 'O' stands in for physically rotating the device
**XNA behaviour:** On a real phone, physically rotating the device raises
orientation-change hardware events that XNA's `GraphicsDeviceManager`
responds to automatically (subject to `SupportedOrientations`); the sample
itself never explicitly triggers a rotation.
**CNA port behaviour:** This desktop has no physical rotation sensor, so
`O` toggles Landscape <-> Portrait and resizes the back buffer to match
(`CycleOrientation()`), the same established pattern as DynamicMenu's
(#077) `O` orientation toggle. Only takes effect while unlocked, matching
how a real locked phone ignores physical rotation.

Fixed during interactive testing: an earlier version cycled through all
three `DisplayOrientation` values (`LandscapeLeft -> LandscapeRight ->
Portrait -> ...`), matching the flag enum's full value set, but the
`directions` texture is drawn with no rotation transform (same as the
original -- see the "Scenario #4" section above), so `LandscapeLeft` and
`LandscapeRight` render pixel-identical at identical dimensions. Every
other `O` press looked like a no-op. Changed to a plain two-state
Landscape/Portrait toggle, matching DynamicMenu's own toggle shape exactly
-- every press now visibly resizes the window. (Screenshot-confirmed
2026-07-05: 800x480 -> 480x800 -> would return to 800x480 on the next
press, same code path.)

Also confirmed live: while `orientationLocked_` is true (toggled by a
single tap/click), `O` correctly has no effect, matching the original's
"a locked phone ignores physical rotation" semantics -- this is *not* a
bug, but it can look like one if orientation gets locked by an incidental
click before `O` is tried (this happened during this session's own
interactive testing, on a shared desktop where a stray click landed on the
window between launch and the first deliberate `O` press).
**Root cause:** No rotation hardware on desktop Linux (the toggle); the
apparent "does nothing" reports were the original's own by-design lock
behavior, not a defect.
**Tracked in:** Same precedent as `samples/DynamicMenu/missing.md`.

## Not preserved: `TargetElapsedTime` (30 fps) and `IsFullScreen = true`
**XNA behaviour:** The constructor sets `TargetElapsedTime =
TimeSpan.FromTicks(333333)` unconditionally (comment: "Frame rate is 30 fps
by default for Windows Phone") and `graphics.IsFullScreen = true`
unconditionally (comment: "Switch to full screen mode") -- neither is `#if
WINDOWS_PHONE`-gated, so both would also apply to a desktop Windows build of
this sample.
**CNA port behaviour:** `OrientationGame`'s constructor never calls
`setTargetElapsedTimeProperty`, so the game runs at CNA's 60 fps default
instead of the original's 30 fps, and explicitly calls
`graphics_.setIsFullScreenProperty(false)` (actively windowed, not just left
at a default) -- matching the windowed-for-desktop-testing precedent already
documented in `samples/DynamicMenu/missing.md`.
**Root cause:** Desktop dev-loop practicality for `IsFullScreen`, matching
repo-wide convention. The 30 fps `TargetElapsedTime` was not carried over the
way it was in `DynamicMenu`/`UISample` (both of which do preserve it) --
flagged here as a real, if minor, timing difference rather than a deliberate
choice.
**Tracked in:** Not planned for `IsFullScreen`. The dropped `TargetElapsedTime`
is worth fixing (a one-line `setTargetElapsedTimeProperty(System::TimeSpan::
FromTicks(333333))` in the constructor) if this sample is revisited.

## Font substitution: Segoe UI Mono -> DejaVu Sans Mono
**XNA behaviour:** `Font.spritefont` specifies "Segoe UI Mono", Regular,
14pt.
**CNA port behaviour:** Generated from DejaVu Sans Mono at 14px via
`tools/make_font.py` (CNA has no `.spritefont`/TTF-at-runtime pipeline).
Glyph metrics differ slightly from the original.
**Root cause:** XNA `.xnb` SpriteFont binaries are not supported; Segoe UI
Mono is not available as an open TTF, so DejaVu Sans Mono is substituted per
this project's established convention (see CLAUDE.md's Assets section).
**Tracked in:** Not planned -- same class of adaptation as
`samples/DynamicMenu/missing.md`'s own Segoe UI Mono -> DejaVu Sans Mono
note.

## Missing: back-buffer size reassertion when locking orientation
**XNA behaviour:** When locking, `Update()` explicitly reasserts the *live*
viewport size before the implicit `ApplyChanges()` later in the same block:
`graphics.SupportedOrientations = Window.CurrentOrientation;` followed by
`graphics.PreferredBackBufferWidth = GraphicsDevice.Viewport.Width;` and
`graphics.PreferredBackBufferHeight = GraphicsDevice.Viewport.Height;`, with a
comment explaining `ApplyChanges()` could otherwise revert to a stale
previously-set preferred size.
**CNA port behaviour:** The lock branch only calls
`graphics_.setSupportedOrientationsProperty(currentOrientation_);` -- the two
`PreferredBackBufferWidth`/`Height` reassertion lines are absent, even though
the adjacent code comment ("keep the current back-buffer size exactly as it
is") claims the original's full behavior. Harmless on CNA's current desktop
backend: `GraphicsDeviceManager`'s orientation-locking is only ever active on
iOS/Android, and CNA's `GameWindow::INTERNAL_OnClientSizeChanged` deliberately
never calls `ApplyChanges()` on a user window resize, so
`PreferredBackBufferWidth`/`Height` can't drift out from under
`CycleOrientation()`'s own last-set value on this backend today -- but it is
a genuine omission of the original's defensive code, not merely a stale
comment.
**Root cause:** Porting oversight -- the two reassertion lines were dropped
when translating the lock branch.
**Tracked in:** Not planned -- currently a dead-code-equivalent gap on
desktop backends; worth revisiting only if a future CNA backend makes
`PreferredBackBufferWidth`/`Height` actually drift between `ApplyChanges()`
calls.

## Added: Escape key also exits the game
**XNA behaviour:** `Update()` exits only on
`GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed`.
**CNA port behaviour:** Also exits on
`Keyboard::GetState().IsKeyDown(Keys::Escape)`.
**Root cause:** Desktop dev-loop practicality -- no gamepad guaranteed
attached.
**Tracked in:** Not planned -- minor, deliberate desktop-usability addition,
same class as `samples/SoccerPitch/missing.md`'s and
`samples/DynamicMenu/missing.md`'s equivalent notes.

## Mouse click synthesizes a Tap gesture
**XNA behaviour:** All interaction is via `TouchPanel` gestures; no mouse
support (Windows Phone only, no Windows/Xbox build of this sample ever
existed -- confirmed via `<XnaPlatform>Windows Phone</XnaPlatform>` and a
single `.csproj`).
**CNA port behaviour:** A left-click rising edge is pushed into CNA's real
gesture queue via `TouchPanel::EnqueueGesture(...)`, so the original's own
`while (TouchPanel.IsGestureAvailable) { ReadGesture() ... }` loop runs
completely unmodified -- CNA does not synthesize touch/gesture events from
mouse input itself.
**Root cause:** No touchscreen on this desktop.
**Tracked in:** Same precedent as DynamicMenu/NinjAcademy's mouse-tap
fallback.

## Verification: idle rendering and 'O' orientation toggle confirmed live
**What was checked:** Built and ran `Orientation_cna_samples` under
`SDL_VIDEODRIVER=x11`. An idle screenshot (no synthetic input sent before
capture) confirms correct rendering: cornflower-blue background, the
"directions" compass texture centered in the viewport, and the
"Orientation: Unlocked" / "Tap to lock orientation." / "('O' simulates
physically rotating the device.)" status text. After confirming genuine
window focus (`xdotool getactivewindow` resolving to this sample's own
"Game" window, re-checked immediately before sending input, per the
`feedback_xdotool_shared_desktop` gotcha), pressing `O` was confirmed live:
window resized 800x480 -> 480x800, texture recentered correctly, status
text unaffected. This same pass is what caught and fixed the
LandscapeLeft/LandscapeRight no-visible-effect bug above, and confirmed the
lock-gates-`O` behavior is correct-by-design (see above).
**What was not separately re-confirmed:** A full cycle back to landscape,
and the mouse-click-to-lock/unlock path in isolation (only observed
indirectly, via an incidental stray click during testing that locked the
sample and initially looked like a bug -- see above). Both use the exact
same confirmed-working code paths (`ApplyChanges()`/`TouchPanel::EnqueueGesture`),
so this is a completeness note, not an open question. A deliberate
full-cycle pass is still owed once input reliably reaches
sample windows again.
