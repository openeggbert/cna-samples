# Missing / Differences from XNA 4.0 original

## Added: keyboard + mouse fallback for the two virtual thumbsticks
**XNA behaviour:** The original "TouchThumbSticks" sample reads exclusively from raw
`TouchPanel` touch points — the left half of the screen is the movement stick, the right
half is the aim/fire stick, and both are meant to be used simultaneously with two thumbs.
**CNA port behaviour:** Real multi-touch is ported faithfully in `VirtualThumbsticks::Update()`
and is the primary path on touch hardware. Since this fundamentally needs *two independent,
simultaneous* contact points — unlike a single-pointer gesture sample, there is no reasonable
single-mouse substitute for "both sticks at once" — a keyboard/mouse fallback was added
instead: WASD/arrow keys drive the left (movement) stick, and the mouse position relative to
the screen center drives the right (aim + auto-fire) stick, with a small deadzone before the
stick registers any magnitude and a capped "full throw" distance. Both fall back
independently and only when their half isn't currently touched, so real touch input on
actual hardware is unaffected. Verified via `xdotool`: WASD movement, mouse-aim ship
rotation, auto-fire while aiming, enemy spawn/homing, world border, and the F1 help overlay
all confirmed working by screenshot.
**Root cause:** No touchscreen on the development machine, and CNA does not synthesize
touch events from mouse input (only real SDL finger events feed `TouchPanel`, confirmed in
`SdlInputBridge.cpp`).
**Tracked in:** Not planned — a deliberate, documented per-sample addition, matching the
precedent set by Yacht #071 and GesturesSample #079.

## No known differences beyond the above
Ship movement/drag physics, aim/fire thresholds and cooldown, enemy spawning and homing,
bullet-enemy collision, the camera-follow transform, starfield, and world border are a
direct, faithful port of `TouchThumbsticksGame.cs`, `PlayerShip.cs`, `EnemyShip.cs`,
`Ship.cs`, and `Bullet.cs`. Screen resolution (800x480) and the fixed 30 fps timestep match
the original's implicit WP7 defaults.
