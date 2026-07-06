# Missing / Differences from XNA 4.0 original

## SpriteFont / HUD labels omitted
**XNA behaviour:** Draws slider bars (via a 1x1 white pixel texture) and slider labels ("Detection Distance:", "Separation  Distance:") using `HUDFont.spritefont` and `SpriteBatch.DrawString`, including a pulsing red/white tint on the currently-selected slider's label.
**CNA port behaviour:** No slider bars, no text HUD, and no pulsing-tint label are drawn at all; the F1 help overlay is the only on-screen control reference.
**Root cause:** Porting simplification — **not** a current CNA framework limitation. DEFERRED.md items 2 (SpriteFont Loading) and 8 (SpriteBatch.DrawString) are both marked ✅ RESOLVED: CNA has a real SpriteFont (`.font.json` + atlas PNG via `tools/make_font.py`) and `SpriteBatch.DrawString` is fully implemented, already used by other samples (e.g. SafeArea, InputSequence). The HUD was simply never (re)implemented in this port after SpriteFont support landed.
**Tracked in:** Not tracked as a CNA gap — could be added as a follow-up port improvement using `tools/make_font.py`; the F1 help overlay is the current substitute.

## Xbox controller button textures omitted
**XNA behaviour:** Draws B/X/Y button icons next to their control descriptions on Windows/Xbox.
**CNA port behaviour:** Button icon textures are not loaded or drawn (no HUD).
**Root cause:** Porting simplification, omitted together with the rest of the HUD (see "SpriteFont / HUD labels omitted" above) — not blocked by any current CNA gap.
**Tracked in:** Not planned independently — would be re-added together with the HUD if that is ever implemented.

## Touch input omitted
**XNA behaviour:** Phone version uses touch gestures (tap buttons, drag sliders, tap to move cat).
**CNA port behaviour:** Desktop keyboard/gamepad only.
**Root cause:** Phone-specific input not applicable to desktop.
**Tracked in:** Not planned.

## Gamepad trigger slider control omitted
**XNA behaviour:** `InputState.SliderMove` falls back to `-CurrentPadState.Triggers.Left + CurrentPadState.Triggers.Right` when neither D-pad-left/right nor keyboard Left/Right are pressed, letting the analog triggers drag the selected slider.
**CNA port behaviour:** `InputState::SliderMove()` returns `0.0f` in that case — only keyboard arrow keys and D-pad left/right move the slider; the analog triggers do nothing.
**Root cause:** Simplification during porting; CNA's `GamePadState` does implement `getTriggersProperty()`, so this is not a framework limitation.
**Tracked in:** not planned

## Bird vertical movement is not doubled (original XNA bug not reproduced)
**XNA behaviour:** `Bird.Update` applies `location = location + moveAmount` (where `moveAmount = direction * moveSpeed * elapsedTime`, updating both X and Y), then afterwards executes `location.Y += direction.Y * moveSpeed * elapsedTime` a second time before wrapping Y. This double-applies the Y component of movement each frame, so birds in the original sample drift roughly 2x faster vertically than horizontally — an apparent bug in the original Microsoft sample code.
**CNA port behaviour:** `Bird::Update` applies `location_.X += ...` and `location_.Y += ...` exactly once each, so birds move at the same speed in both axes (no vertical speed-up).
**Root cause:** Deliberate non-reproduction of an apparent upstream bug; the port implements the evidently-intended single-application movement instead.
**Tracked in:** not planned (intentional deviation — see CLAUDE.md porting philosophy on stray from original only with concrete reason)

## Flock/cat movement bounds use full viewport instead of TitleSafeArea
**XNA behaviour:** `SpawnFlock` and `ToggleCat` construct `Flock`/`Cat` with `GraphicsDevice.Viewport.TitleSafeArea.Width` / `.Height` as the wrap/clamp boundary, so birds and the cat are confined to the TV-safe rectangle (inset from the screen edges), not the full backbuffer.
**CNA port behaviour:** `FlockingSampleGame::LoadContent`/`Update` pass `getViewportProperty().getWidthProperty()` / `getHeightProperty()` (the full viewport) to `Flock` and `Cat`, so animals wrap/clamp at the literal screen edges instead of the inset safe area.
**Root cause:** Porting simplification — not a CNA framework limitation; CNA does implement `Viewport::getTitleSafeAreaProperty()` and it is used by several other ported samples (Pathfinding, SafeArea, Platformer, PerformanceMeasuring, CardsStarterKit, Yacht).
**Tracked in:** not planned (cosmetic on desktop, where overscan safe-area insets are not meaningful; full-viewport bounds are effectively equivalent since CNA renders 1:1 on a desktop window)
