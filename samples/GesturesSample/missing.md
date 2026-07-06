# Missing / Differences from XNA 4.0 original

## Adapted: windowed instead of `IsFullScreen = true`
**XNA behaviour:** The constructor sets `graphics.IsFullScreen = true;` (Windows
Phone fills the screen at its native 800x480 resolution; the frame rate is also
set to 30 fps via `TargetElapsedTime = TimeSpan.FromTicks(333333)`).
**CNA port behaviour:** `PreferredBackBufferWidth/Height` are explicitly set to
800x480 and the 30 fps `TargetElapsedTime` is preserved, but `IsFullScreen` is
left at its default (windowed) — matching this project's established
DynamicMenu/HoneycombRush/Bounce/TicTacToe/PathDrawing precedent of leaving
desktop ports windowed.
**Root cause:** Desktop dev-loop practicality; forcing fullscreen has no
behavioral benefit on desktop and would make screenshotting/testing this
sample inconsistent with the rest of the project.
**Tracked in:** Not planned.

## Texture converted from TGA to PNG
**XNA behaviour:** Loads `cat.tga` via `Content.Load<Texture2D>("cat")`.
**CNA port behaviour:** Converted to `Content/Images/cat.png`, loaded via
`Content.Load<Texture2D>("Images/cat")`.
**Root cause:** CNA's `ContentManager`/asset pipeline does not support `.xnb`/TGA
source assets (see CLAUDE.md Assets section); source art was converted to PNG,
matching the same TGA→PNG conversion documented for Audio3D's `CatTexture.tga`.
**Tracked in:** Not planned — standard asset-conversion step for this project.

## Added: parallel mouse input path
**XNA behaviour:** The original "TouchGestureSample" is Windows Phone 7 only and reads
exclusively from `TouchPanel` (raw touch points for selection, gestures for Hold, Tap,
DoubleTap, FreeDrag, Flick, and Pinch).

**CNA port behaviour:** `TouchPanel`/gesture handling is ported faithfully and is the
primary input path (real multi-touch hardware works exactly as in the original — see
Yacht #071 for prior end-to-end verification of CNA's touch/gesture pipeline). Since this
development machine has no touchscreen, a second, parallel mouse path was added so the
sample is playable and screenshot-verifiable on a desktop: left-click-drag moves the
selected sprite, a quick click changes its color (Tap), press-and-hold creates/removes a
sprite (Hold), releasing mid-drag throws it with the drag's velocity (Flick), and the
scroll wheel scales the selected sprite in place of a two-finger Pinch. Both paths share
the same sprite-selection state and were verified independently by simulated input
(`xdotool`) and screenshots: hold-create, tap-color-cycle, drag-move, drag-release-flick
(with wall-bounce physics), scroll-to-scale, and hold-to-remove all confirmed working.
**Root cause:** No touch hardware on the development/desktop target; CNA does not
synthesize touch events from mouse input (confirmed in `SdlInputBridge.cpp` — only real
SDL finger events feed `TouchPanel`), so a sample-side fallback is needed for desktop use,
matching the precedent set by Yacht #071.
**Tracked in:** Not planned — this is a deliberate, documented per-sample addition, not a
CNA gap.

## Approximation: mouse Hold-timer and Pinch-scale are not independent
**XNA behaviour:** Hold and Pinch are recognized as independent, simultaneous-capable
gestures by the real multi-touch `GestureDetector` (Pinch requires two fingers, Hold
requires one held finger; they cannot conflict for the same contact).
**CNA port behaviour:** The mouse fallback approximates both with a single left mouse
button, so holding the button down accumulates a Hold timer even while scrolling to
scale. Scroll input marks the interaction as "active manipulation" (same as a drag) so it
suppresses an accidental Hold-fire while scaling — but the two are still emulated through
one physical control, unlike real two-finger multi-touch.
**Root cause:** A single mouse pointer cannot represent two independent touch contacts;
this is an inherent limitation of the desktop fallback, not of CNA's real `TouchPanel`.
**Tracked in:** Not planned — accepted approximation of the added mouse path above.

## No known differences beyond the above
Gesture recognition, sprite creation/removal/coloring/movement/scaling, and the
wall-bounce/friction physics are otherwise a direct, faithful port of `Game1.cs` and
`Sprite.cs`. Screen resolution (800x480, matching the WP7 native resolution the
original relied on implicitly via `IsFullScreen = true`, see above) and the fixed
30 fps timestep are preserved.
