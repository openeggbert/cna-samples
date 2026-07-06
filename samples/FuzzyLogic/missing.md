# Missing / Differences from XNA 4.0 original

## Fuzzy weight bar labels omitted
**XNA behaviour:** Each bar (Distance, Angle, Time) has a text label ("Distance"/"Angle"/"Time") drawn to its left using `font.MeasureString` + `spriteBatch.DrawString(font, ...)`, loaded from `Content.Load<SpriteFont>("hudFont")`.
**CNA port behaviour:** The coloured bars are drawn but without labels or any `SpriteFont` load; `DrawBar()` has a comment "DrawString omitted — CNA has no SpriteFont support yet".
**Root cause:** Stale — CNA now fully supports `SpriteFont`/`DrawString` (`.font.json` + atlas PNG, see DEFERRED.md item 2, resolved; used by other samples e.g. SafeArea, InputSequence). This port was written before that support landed and was never updated; it is a port-time omission, not a current framework gap.
**Tracked in:** not planned (straightforward follow-up port work, not a CNA limitation)

## Touch input (tap-to-select bar, drag-to-adjust) omitted
**XNA behaviour:** On Windows Phone, `TouchPanel.GetState()` is polled each frame. A `Pressed` touch inside one of the three bar rectangles selects that weight; while dragging, horizontal movement (`Moved`) increases/decreases the selected weight in the direction of the drag; `Released` clears the drag state.
**CNA port behaviour:** No touch handling at all — only keyboard and gamepad.
**Root cause:** Touch input is phone-specific; not wired up for this desktop port.
**Tracked in:** not planned

## Gamepad D-Pad-Up/Down and thumbstick-tilt weight selection omitted
**XNA behaviour:** `HandleInput()` cycles `currentlySelectedWeight` on an edge-triggered press of `Keys.Up`/`Keys.Down` **or** `Buttons.DPadUp`/`Buttons.DPadDown` **or** `Buttons.LeftThumbstickUp`/`Buttons.LeftThumbstickDown`.
**CNA port behaviour:** `HandleInput()` only checks the edge-triggered keyboard `Keys::Up`/`Keys::Down`; no gamepad button cycles the selection (the gamepad thumbstick X-axis and D-Pad left/right are still used to adjust the selected weight's value, matching the original).
**Root cause:** Port omission — CNA's `Buttons` enum does define `DPadUp`/`DPadDown`/`LeftThumbstickUp`/`LeftThumbstickDown` (see `Buttons.hpp`), so this is not a framework gap.
**Tracked in:** not planned

## Mouse class renamed to MouseEntity
**XNA behaviour:** The prey entities are a class named `Mouse` in namespace `FuzzyLogic`.
**CNA port behaviour:** The class is named `MouseEntity` to avoid collision with `Microsoft::Xna::Framework::Input::Mouse` which is brought into scope via `using namespace`.
**Root cause:** C++ name collision between two distinct types named `Mouse`.
**Tracked in:** not planned (C++ language constraint)

## Viewport.TitleSafeArea not called; port hardcodes the equivalent 20px inset
**XNA behaviour:** `Initialize()` sets `levelBoundary = GraphicsDevice.Viewport.TitleSafeArea;` and then manually insets it further (`X += 20; Y += 20; Width -= 40; Height -= 40;`). Per FNA (the authoritative XNA 4.0 desktop reference) and CNA's own `Viewport::getTitleSafeAreaProperty()` (`Viewport.cpp`), `TitleSafeArea` simply returns `Bounds` unmodified on this platform (no TV-overscan margin) — so the original's effective level boundary works out to `Rectangle(20, 20, width-40, height-40)`.
**CNA port behaviour:** `Initialize()` computes the level boundary directly as `Rectangle(20, 20, width-40, height-40)`, without calling `getTitleSafeAreaProperty()`.
**Root cause:** Since `TitleSafeArea == Bounds` on this platform (both in FNA and in CNA), the port's hardcoded rectangle is numerically identical to the original's two-step computation (`TitleSafeArea` + manual inset) — there is no behavioral difference. This is a pure code-structure simplification (collapsing an API call plus arithmetic into one literal), not a functional gap. (A prior version of this note incorrectly claimed the results would differ; they do not.)
**Tracked in:** not planned (no observable behavior difference; purely a code-structure simplification)

## Texture assets converted from TGA to PNG
**XNA behaviour:** `mouse.tga` and `tank.tga` are the source art in `FuzzyLogicContent/`, compiled to `.xnb` and loaded via `Content.Load<Texture2D>("Mouse")` / `Content.Load<Texture2D>("Tank")`.
**CNA port behaviour:** Converted to `Content/mouse.png` and `Content/tank.png`, loaded via `Content.Load<Texture2D>("mouse")` / `Content.Load<Texture2D>("tank")`.
**Root cause:** CNA's asset pipeline does not support `.xnb`/TGA source assets (see CLAUDE.md Assets section); same TGA→PNG conversion documented for Audio3D's `CatTexture.tga` and GesturesSample's `cat.tga`.
**Tracked in:** not planned — standard asset-conversion step for this project.

## OnePixelWhite texture loaded from Content instead of generated at runtime
**XNA behaviour:** `LoadContent()` creates the 1x1 white pixel procedurally: `onePixelWhite = new Texture2D(GraphicsDevice, 1, 1, false, SurfaceFormat.Color); onePixelWhite.SetData<Color>(new Color[] { Color.White });`. (An `OnePixelWhite.png` file is present in the Content project but is never actually loaded by the game code — it appears to be vestigial.)
**CNA port behaviour:** `LoadContent()` calls `getContentProperty().Load<Texture2D>("OnePixelWhite")`, loading `Content/OnePixelWhite.png` from disk instead of constructing the texture programmatically.
**Root cause:** Port-time simplification — CNA's `Texture2D` fully supports the constructor + `SetData` pattern used by the original (the Pathfinding sample's port uses this exact pattern faithfully for its own one-pixel-white texture). This port used a pre-baked PNG asset instead. The rendered result (a single opaque white pixel, stretched to fill each bar rectangle) is visually identical either way.
**Tracked in:** not planned (cosmetic/structural only; straightforward follow-up port work, not a CNA limitation)
