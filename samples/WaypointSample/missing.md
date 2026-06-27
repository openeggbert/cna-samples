# Missing / Differences from XNA 4.0 original

## SpriteFont / HUD text omitted
**XNA behaviour:** Displays the current behavior type (Linear/Steering) and instructions on screen using DrawString.
**CNA port behaviour:** No text is drawn; the HUD is absent.
**Root cause:** CNA has no SpriteFont support yet.
**Tracked in:** DEFERRED.md

## Touch input replaced by keyboard/gamepad
**XNA behaviour:** Touch screen used to place waypoints; designed for Windows Phone.
**CNA port behaviour:** Arrow keys move the cursor; A = place waypoint, B = cycle behavior, X = reset.
**Root cause:** Phone-specific input not applicable to desktop.
**Tracked in:** Not planned (intentional desktop adaptation).
