# Missing / Differences from XNA 4.0 original

## SpriteFont / state text omitted
**XNA behaviour:** Loads `Content.Load<SpriteFont>("hudFont")` and draws "Tank State:
Chasing/Wander/Caught" and "Mouse State: Evading/Wander" at (50,50)/(50,75) with a
black+white drop-shadow using `DrawString`.
**CNA port behaviour:** No `hudFont` asset is generated and no text is drawn; only the
tank/cat/mouse sprites and the F1 help overlay are rendered.
**Root cause:** Not a CNA limitation — CNA's `SpriteFont`/`DrawString` support is fully
implemented (`.font.json` + atlas PNG via `tools/make_font.py`, already used by
InputSequence, SafeArea, and other samples). This sample's port simply omits the
on-screen state readout; it could be added the same way those samples do.
**Tracked in:** not planned (cosmetic HUD text only; game behavior is otherwise identical)

## Windows Phone portrait/full-screen branch removed
**XNA behaviour:** `graphics.SupportedOrientations = DisplayOrientation.Portrait` is set
unconditionally in the constructor (inert on Windows/Xbox); a `#if WINDOWS_PHONE` branch
then sets a 480x800 back buffer, full screen, and a fixed `TargetElapsedTime =
TimeSpan.FromTicks(333333)`; the `#else` (Windows/Xbox) branch used here sets 853x480
windowed instead. (The sample contains no vibration/`GamePad.Vibrate` code at all.)
**CNA port behaviour:** Only the non-phone (853x480 windowed) branch is ported; no
orientation/full-screen phone path or fixed-timestep override exists.
**Root cause:** Desktop-only port target; phone-specific XNA platform code is out of
scope.
**Tracked in:** not planned
