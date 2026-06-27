# Missing / Differences from XNA 4.0 original

## SpriteFont / HUD labels omitted
**XNA behaviour:** Draws slider labels ("Detection Distance:", "Separation Distance:") and control hints using DrawString.
**CNA port behaviour:** Slider bars are omitted entirely; no text HUD is drawn.
**Root cause:** CNA has no SpriteFont support yet.
**Tracked in:** DEFERRED.md

## Xbox controller button textures omitted
**XNA behaviour:** Draws B/X/Y button icons next to their control descriptions on Windows/Xbox.
**CNA port behaviour:** Button icon textures are not loaded or drawn (no HUD).
**Root cause:** Without SpriteFont labels the icons alone are meaningless; omitted together with HUD.
**Tracked in:** Not planned independently — resolves with SpriteFont support.

## Touch input omitted
**XNA behaviour:** Phone version uses touch gestures (tap buttons, drag sliders, tap to move cat).
**CNA port behaviour:** Desktop keyboard/gamepad only.
**Root cause:** Phone-specific input not applicable to desktop.
**Tracked in:** Not planned.
