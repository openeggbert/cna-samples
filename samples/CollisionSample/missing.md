# Missing / Differences from XNA 4.0 original

## FPS overlay not rendered
**XNA behaviour:** A `FrameRateCounter` DrawableGameComponent loads a SpriteFont and
draws the frames-per-second count to the screen each frame.
**CNA port behaviour:** The component is registered and counts frames internally, but
nothing is drawn — `SpriteFont` is not yet available in CNA.
**Root cause:** CNA has no SpriteFont / text rendering yet.
**Tracked in:** DEFERRED.md (SpriteFont).

## Touch / gesture input removed
**XNA behaviour:** Tap cycles the camera group, FreeDrag rotates the camera,
Pinch zooms.
**CNA port behaviour:** Touch/gesture input is not implemented. The same actions
are available via keyboard (G, arrow keys, +/-) and gamepad.
**Root cause:** Desktop-only port scope.
**Tracked in:** not planned.

## Phone-specific back-buffer settings omitted
**XNA behaviour:** On Windows Phone the back-buffer is set to fullscreen with 30 fps
target.
**CNA port behaviour:** Windowed 853×480, default 60 fps.
**Root cause:** Phone-specific settings outside the scope of a desktop port.
**Tracked in:** not planned.

## Color named constants replaced with RGBA literals
**XNA behaviour:** Uses `Color.CornflowerBlue`, `Color.White`, `Color.Red`,
`Color.Yellow`, `Color.LightGray`, `Color.Black`.
**CNA port behaviour:** Equivalent RGBA values used directly.
**Root cause:** CNA Color does not expose named static constants yet.
**Tracked in:** CNA issue (minor convenience gap).

## BoundingOrientedBox is a local struct, not an XNA type
**XNA behaviour:** `BoundingOrientedBox` is defined in the sample itself (not in XNA).
**CNA port behaviour:** Same — `BoundingOrientedBox` lives in
`samples/CollisionSample/src/BoundingOrientedBox.hpp`.
**Root cause:** This is faithful to the original; no difference in behaviour.
**Tracked in:** not planned.

## Ray Z-axis test uses Forward instead of Backward
**XNA behaviour:** The OBB ray test iterates over R.Right, R.Up, R.Forward
(where Forward = -Z in XNA's right-handed space).
**CNA port behaviour:** Same — uses `getForwardProperty()` which maps to the same
-Z direction.
**Root cause:** Faithful translation; no behavioural difference.
**Tracked in:** not planned.
