# Missing / Differences from XNA 4.0 original

## Added: mouse fallback for taps, drags, and page flips
**XNA behaviour:** Everything in this sample is touch/gesture-driven —
`MenuScreen` reacts to `Tap`, `ScrollTracker` reacts to raw touch-down plus
`VerticalDrag`/`Flick`/`DragComplete`, `PageFlipTracker` reacts to
`HorizontalDrag`/`Flick`/`DragComplete`. `Mouse` is never read anywhere in the
original.
**CNA port behaviour:** CNA does not synthesize touch/gesture events from
mouse input, and this desktop has no touchscreen. All the mouse-fallback
synthesis is centralized in one place — `InputState::UpdateMouseFallback()` —
rather than scattered across each control: a left-click's rising edge
synthesizes a raw `TouchLocation` (state `Pressed`) in `TouchState` (the
"just touched down" bootstrap signal `ScrollTracker` needs — gestures alone
can't express a zero-delta initial contact) and a `Tap` gesture (for
`MenuScreen`); each held frame's mouse movement synthesizes
`HorizontalDrag`/`VerticalDrag` gestures from the frame-to-frame delta (for
`PageFlipTracker`/`ScrollTracker`); release synthesizes `DragComplete`.
**`Flick` (a fast release while still moving) is deliberately not
approximated** — only a deliberate drag-then-release, which both trackers
already settle/snap correctly on without it. Because the synthesis lives in
`InputState`, none of the ported `Controls`/`ScrollTracker`/`PageFlipTracker`
code needed any CNA-specific changes at all.
**Root cause:** No touchscreen on the development machine.
**Tracked in:** Not planned — deliberate, documented addition, matching the
precedent set by GesturesSample/TouchThumbsticks/SnowShovel/DynamicMenu.

## Simplified: Control/IControl not split; no separate ITextControl
**XNA behaviour:** N/A — this sample's `Controls.Control` was never split
from an interface in the original (unlike DynamicMenu's library, which this
one is unrelated to and structured independently).
**CNA port behaviour:** No change — noted here only because `Controls::Control`
in this sample happens to look structurally similar to DynamicMenu's; they
are two separate, independently-ported control libraries with no code
sharing (matching this project's "no shared sample library" rule), and
neither one's original C# had a redundant interface to begin with.
**Tracked in:** N/A — not an actual difference, just avoiding confusion with
DynamicMenu's own (different) Control/IControl merge.

## Adapted: TimeSpan formatting for fake high-score times
**XNA behaviour:** `HighScorePanel`'s fake leaderboard data formats a
`TimeSpan` with .NET's general format specifier (`"{0:g}"`, e.g. `"1:02:03"`).
**CNA port behaviour:** sharp-runtime's `String::Format` doesn't implement
.NET's TimeSpan format specifiers (only standard numeric ones like `D`/`F`/
`X` apply, and only to numeric types). Manually formatted as `H:MM:SS`
instead — same class of adaptation as SnowShovel's `FormatMinutesSeconds`/
Platformer's `pad2`.
**Root cause:** `String::Format` TimeSpan-format support is a larger feature
than this one sample needs; not escalated to a CNA fix.
**Tracked in:** Not planned.

## Fixed: two unreachable bugs in the original, ported faithfully otherwise
Two small bugs in the original C# were fixed since they're one-line changes
and could otherwise crash or silently misbehave if ever triggered, even
though neither is actually reachable anywhere in this sample as shipped:
- `TextControl.Font`'s getter was `get { return Font; }` (infinite recursion /
  stack overflow) instead of `get { return font; }`. Never actually read
  anywhere in the sample (only ever written to), so never triggered.
- `CommonGraphics.DrawRectangle` took a `color` parameter but never passed it
  to the underlying draw call (hardcoded `Color.White` instead). Neither
  `DrawRectangle` nor `DrawCenteredText` in `CommonGraphics` are actually
  called anywhere in this sample — they're plain utility functions ported
  for completeness alongside everything else in that file.
**Tracked in:** Not planned — trivial, faithful bug fixes with no behavioral
impact on anything the sample actually exercises.

## Dropped: tombstoning (SerializeState/DeserializeState/IsSerializable)
**XNA behaviour:** `ScreenManager.SerializeState()`/`DeserializeState()` save
and restore the screen stack to `IsolatedStorageFile` so a suspended
("tombstoned") Windows Phone app can resume where it left off;
`GameScreen.IsSerializable`/`Serialize()`/`Deserialize()` support this per-screen.
**CNA port behaviour:** Dropped entirely, matching the precedent already set
by Yacht (`YachtGame`) and the existing GameStateManagement port — the game
always starts fresh with `BackgroundScreen` + `MainMenuScreen`, exactly like
those samples do. `TraceEnabled`/`TraceScreens()` (a `Debug.WriteLine`
screen-stack dumper) is dropped too, for the same reason: a Windows-Phone
app-lifecycle concept the OS never invokes here, and a debug helper nothing
in the sample enables.
**Root cause:** No tombstoning concept on desktop; not exercised by
anything visible in this sample regardless.
**Tracked in:** Not planned.

## Adapted: BackgroundScreen uses the shared ContentManager, not a private one
**XNA behaviour:** `BackgroundScreen` creates its own private `ContentManager`
so its (large) background texture can be unloaded independently of the
game's main content, before entering gameplay.
**CNA port behaviour:** Uses the shared `ContentManager` like every other
screen, matching the precedent already set by the existing GameStateManagement
port's own `BackgroundScreen`. There's no long-running gameplay content to
free memory for in this sample (or in this project generally).
**Root cause:** Simplification matching established project precedent; no
behavioral difference visible to the user.
**Tracked in:** Not planned.

## Verification note
Interactively confirmed by screenshot: Main Menu renders correctly (title,
both menu entries, stretched background); tapping "Select level" via the
mouse fallback correctly transitions through `LoadingScreen` into
`LevelSelectScreen`, showing the first level page ("House") with its
background image, title, and description all correctly positioned; a
horizontal mouse-drag correctly triggers `PageFlipTracker`'s flip-to-next-page
logic, transitioning smoothly to the next level ("Pasture"). This also
caught and fixed a real bug: `TouchPanel::DisplayWidth`/`DisplayHeight` were
never set anywhere in this sample (an omission on my part, not inherent to
the port), so `PageFlipTracker`'s drag-to-flip threshold computed against 0
and no drag could ever cross it — fixed in `UISampleGame::Initialize()` using
the known default back-buffer constants (800x480), the same pattern as
SnowShovel's `Initialize()`-time viewport workaround, since this sample never
overrides `PreferredBackBufferWidth`/`Height` (see NEXT.md section 5's
viewport-timing gotcha for why querying the viewport directly isn't safe
here either).
`ScrollTracker`/`HighScoreScreen`'s vertical-scroll behavior still could not be
interactively confirmed in a follow-up session, despite the Main Menu → "High
scores" navigation itself working correctly (tap-to-select, confirmed by
screenshot). The blocker this time was different: temporary debug
instrumentation in `InputState::UpdateMouseFallback()` showed that while a
mouse button is held down via `xdotool mousedown`/`mousemove`/`mouseup`, the
game's polled `Mouse::GetState()` position freezes at the press location for
the whole hold — `xdotool getmouselocation` confirms the real X11 pointer
keeps moving the entire time, but the in-game value doesn't change until the
button is released, at which point it immediately jumps to the correct final
position. Mouse motion with no button held updates every frame correctly (also
confirmed with the same instrumentation). This looks like an X11 pointer-grab
interaction specific to this desktop's window manager/compositor (see NEXT.md
section 5's `mutter-x11-frames`/focus gotchas), not a bug in `ScrollTracker`,
`ScrollingPanelControl`, or `InputState` — all three were re-read line by line
this session with no logic issue found, and SnowShovel's analogous
click-and-drag fallback was already confirmed working on real hardware in an
earlier session. Tracked in NEXT.md section 4C/5; a real mouse or touchscreen
would be needed for a definitive interactive confirmation.
