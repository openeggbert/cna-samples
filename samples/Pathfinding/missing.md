# Missing / Differences from XNA 4.0 original

## HUD text labels omitted (SpriteFont not available)
**XNA behaviour:** Draws text labels next to each button icon ("Start/Stop", "Reset",
"Next map", "Pathfinding mode: ..."), a "Time Step:" label, and a path search status
string ("Not Searching", "Searching...", "Path Found!", "No Path Found!") using
`SpriteFont`.
**CNA port behaviour:** Button icons (A/B/X/Y) and the time-step slider bar are
drawn, but all text labels are omitted.
**Root cause:** CNA has no SpriteFont support yet.
**Tracked in:** DEFERRED.md.

## Touch input omitted
**XNA behaviour:** Supports Windows Phone touch gestures (Tap) for all UI buttons
and raw touch for the time-step slider.
**CNA port behaviour:** Keyboard only (A=Start/Stop, B=Reset, X=Next search type,
Y=Next map, Left/Right=adjust time step).
**Root cause:** CNA targets desktop; no touch panel abstraction.
**Tracked in:** not planned.

## Custom Content Pipeline type replaced by direct XML parsing
**XNA behaviour:** `Content.Load<MapData>("map1")` uses the `PathfindingData`
Content Pipeline extension to deserialise the XNA XML format.
**CNA port behaviour:** The XML files are parsed directly at runtime using simple
string extraction.
**Root cause:** CNA does not support Content Pipeline extensions.
**Tracked in:** not planned.

## All Draw calls merged into a single SpriteBatch Begin/End per frame
**XNA behaviour:** Each subsystem (`Map`, `PathFinder`, `Tank`, `WaypointList`,
`DrawHUD`, `DrawPathStatus`) calls `SpriteBatch.Begin()` / `End()` independently.
**CNA port behaviour:** A single `Begin()` / `End()` wraps all sprite draws.
All `Draw()` methods assume the batch is already started.
**Root cause:** CNA Vulkan backend discards all but the last Begin/End pair per frame
(see `../cna/known_bugs.md`).
**Tracked in:** CNA known_bugs.md.

## Color named constants replaced with RGBA literals
**XNA behaviour:** Uses `Color.Navy`, `Color.LightBlue`, `Color.Green`, etc.
**CNA port behaviour:** Equivalent RGBA values used inline.
**Root cause:** CNA Color does not expose named static constants yet.
**Tracked in:** CNA issue (minor convenience gap).
