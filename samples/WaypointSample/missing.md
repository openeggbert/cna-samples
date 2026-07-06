# Missing / Differences from XNA 4.0 original

## SpriteFont / HUD text omitted
**XNA behaviour:** Draws a persistent HUD every frame via `DrawString`: `"Behavior Type: " + tank.BehaviorType` near the top-left corner, plus a static block of control instructions ("Use the arrow keys to move the cursor", etc.) at a fixed position.
**CNA port behaviour:** Neither string is drawn. The instructions are instead covered by the CNA-only F1 help overlay (`Content/help.png`, shown for 10 seconds on keypress), but there is no on-screen equivalent of the live "Behavior Type: Linear/Steering" label — the player must remember which mode is active or infer it from the tank's turning behavior.
**Root cause:** Port gap, not a CNA limitation — CNA's `SpriteFont`/`SpriteBatch.DrawString` is fully implemented (DEFERRED.md item 2, ✅ resolved); this sample never had a `.font.json`/atlas generated (no `HUDFont.spritefont` equivalent exists under `Content/`), so the persistent behavior-type label was dropped rather than ported.
**Tracked in:** not planned (cosmetic; F1 overlay covers the one-time instructions, only the live mode indicator is missing).

## Touch input / Windows Phone menu bar removed
**XNA behaviour:** `#if WINDOWS_PHONE` branches (not compiled on Windows/Xbox, which is what this port is based on) add: touch-panel waypoint placement and multi-touch behavior cycling, an on-screen menu bar with "Clear" and behavior-cycle buttons drawn via a blank texture, and a `RenderTarget2D` trick that renders the whole scene into an 800x480 buffer and redraws it rotated 90 degrees (`MathHelper.PiOver2`) for the phone's portrait screen.
**CNA port behaviour:** None of this is present. Arrow keys move the cursor; A = place waypoint, B = cycle behavior, X = reset — matching the non-phone (`Windows`/Xbox) desktop code path of the original exactly.
**Root cause:** Phone-specific input/rendering not applicable to a desktop port.
**Tracked in:** Not planned (intentional desktop adaptation; the desktop-path C# code has no touch/menu-bar/rotation logic either).

## Tank ported as a plain class, not a DrawableGameComponent
**XNA behaviour:** `Tank : DrawableGameComponent` is added via `Components.Add(tank)` in the `WaypointSample` constructor; `Game.Components` automatically calls `Tank.Update`/`Tank.Draw` each frame, and `Tank.Draw` opens its own private `SpriteBatch` (`Begin()`/`End()`) separate from the main game's `spriteBatch`. `WaypointList.Draw` likewise opens and closes its own `Begin()`/`End()` on that same tank-owned `SpriteBatch` before `Tank.Draw`'s own `Begin()`/`End()` runs — three separate Begin/End cycles per frame across two `SpriteBatch` instances.
**CNA port behaviour:** `Tank` is a plain member of `WaypointSampleGame` (not a `GameComponent`); `Update`/`Draw` are called explicitly from the game's own `Update`/`Draw`. All drawing (waypoints, tank, cursor, help overlay) happens inside one shared `SpriteBatch` with a single `Begin()`/`End()` pair per frame — `Tank::Draw`/`WaypointList::Draw` assume the batch is already begun and never call `Begin`/`End` themselves.
**Root cause:** CNA does implement `DrawableGameComponent`/`Game.Components` (not a blocker here), so this is a deliberate simplification. Consolidating to a single `SpriteBatch::Begin()/End()` per frame also sidesteps CNA's Vulkan-backend bug where a second `Begin()/End()` pair in the same frame discards the first (see `samples/ParticleSample/missing.md`); the default EasyGL backend does not have this bug, but merging batches is harmless and matches the pattern used elsewhere in this repo.
**Tracked in:** not planned (refactored, functionally equivalent; same precedent as `samples/ParticleSample/missing.md`'s "DrawableGameComponent not used").
