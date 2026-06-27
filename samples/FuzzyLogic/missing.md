# Missing / Differences from XNA 4.0 original

## Fuzzy weight bar labels omitted
**XNA behaviour:** Each bar (Distance, Angle, Time) has a text label drawn to its left using `DrawString` and `SpriteFont`.
**CNA port behaviour:** The coloured bars are drawn but without labels.
**Root cause:** CNA has no SpriteFont support yet.
**Tracked in:** DEFERRED.md

## Mouse class renamed to MouseEntity
**XNA behaviour:** The prey entities are a class named `Mouse` in namespace `FuzzyLogic`.
**CNA port behaviour:** The class is named `MouseEntity` to avoid collision with `Microsoft::Xna::Framework::Input::Mouse` which is brought into scope via `using namespace`.
**Root cause:** C++ name collision between two distinct types named `Mouse`.
**Tracked in:** not planned (C++ language constraint)

## Viewport.TitleSafeArea replaced by full viewport minus margin
**XNA behaviour:** `Initialize()` uses `GraphicsDevice.Viewport.TitleSafeArea` as the level boundary.
**CNA port behaviour:** The level boundary is `Rectangle(20, 20, width-40, height-40)` — equivalent inset on a standard display.
**Root cause:** CNA Viewport does not expose `TitleSafeArea`.
**Tracked in:** CNA issue
