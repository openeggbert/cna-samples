# Missing / Differences from XNA 4.0 original

## HUD text labels omitted
**XNA behaviour:** Draws text labels next to each button icon ("Start/Stop", "Reset",
"Next map", "Pathfinding mode: ..."), a "Time Step:" label, a "Search Steps: {n}"
counter, and a path search status string ("Not Searching", "Searching...",
"Path Found!", "No Path Found!") using `SpriteFont`/`DrawString`.
**CNA port behaviour:** Button icons (A/B/X/Y) and the time-step slider bar are
drawn, but all text labels are omitted.
**Root cause:** Stale — at the time this sample was ported, CNA had no SpriteFont
support. CNA now has a full `SpriteFont`/`DrawString` implementation
(`.font.json` + atlas PNG; see DEFERRED.md items 2 and 8, both ✅ resolved), but
this port has not been revisited to load a font and add the missing `DrawString`
calls.
**Tracked in:** not yet ported (port maintenance backlog, not a CNA limitation).

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
**CNA port behaviour:** Equivalent RGBA values used inline (with a comment noting
the intended named color), e.g. `Color(0, 0, 128, 255) // Navy`.
**Root cause:** Stale — CNA's `Color` class now exposes the full set of named
static constants (`Color::Navy`, `Color::LightBlue`, `Color::Green`, etc.; see
`cna/include/Microsoft/Xna/Framework/Color.hpp`), but this port predates that and
has not been updated to use them.
**Tracked in:** not yet ported (cosmetic-only; no functional difference).
