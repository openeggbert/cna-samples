# Missing / Differences from XNA 4.0 original

The original is the **Catapult Wars Lab** training kit
(`/rv/tmp/XNAGameStudio/Samples/CatapultWars_4_0/`), a Windows Phone 7 game. The
complete game ported here is `Source/EX2_PolishAndMenus/End/CatapultGame`.

## Input: touch gestures → mouse + keyboard
**XNA behaviour:** The game is driven entirely by `TouchPanel` gestures —
`FreeDrag`/`DragComplete` to aim and fire the catapult, and `Tap` to select menu
entries, dismiss the instructions screen, and dismiss the end-game screen.
**CNA port behaviour:** CNA has no touch input on desktop, so:
- Aim/fire: press and drag the mouse, then release (drag distance → shot strength).
- Menu selection: Up/Down + Enter, or click an entry.
- "Tap to continue" (instructions, game over): left click or Enter/Space.
- Pause / cancel: Escape (also Back/Start on a gamepad).
**Root cause:** Platform input model difference (Windows Phone touch vs desktop).
**Tracked in:** not planned — desktop has no touch; the mouse mapping is faithful in spirit.

## No verbatim SampleName.htm
**XNA behaviour:** Most XNA samples ship a `SampleName.htm` documentation file that
is copied verbatim and whose "Sample Controls" table drives the F1 help overlay.
**CNA port behaviour:** This training kit ships a Word document
(`2D Game Development With XNA.doc`) instead, so there is no `.htm` to copy. A
`CatapultWars.htm` was **authored** for the port to describe the desktop controls
and to generate `Content/help.png`.
**Root cause:** Training-kit packaging differs from the standard sample packaging.
**Tracked in:** CLAUDE.md "Sample documentation" — documented deviation.

## Catapult animation definitions inlined instead of parsed from XML
**XNA behaviour:** `Catapult.Initialize` loads
`Content/Textures/Catapults/AnimationsDef.xml` at runtime via `XDocument.Load` and
builds the animation table from it.
**CNA port behaviour:** The same definitions are inlined as a small C++ table in
`Catapult::Initialize` (the `AnimationsDef.xml` file is still shipped in `Content/`
for reference). Frame sizes, sheet dimensions, offsets and the "SplitFrame" value
are identical to the XML.
**Root cause:** Avoids pulling an XML parser into a self-contained sample; the data
is fixed.
**Tracked in:** not planned.

## Windows Phone sensors removed (accelerometer / vibration)
**XNA behaviour:** On a missed/hit boulder the game triggers
`VibrateController.Default.Start(...)`.
**CNA port behaviour:** Vibration calls are omitted (desktop has no vibration motor).
No gameplay effect.
**Root cause:** Windows Phone-only hardware API.
**Tracked in:** not planned.

## Asset loading is synchronous (no background thread)
**XNA behaviour:** `InstructionsScreen` spawns a `System.Threading.Thread` to run
`GameplayScreen.LoadAssets` while showing a "Loading..." caption.
**CNA port behaviour:** Assets are loaded synchronously on the tap that leaves the
instructions screen. Loading is fast enough that no threading is needed; the
"Loading..." caption path is retained but effectively never visible.
**Root cause:** Simplification; avoids threading a tiny synchronous load.
**Tracked in:** not planned.

## Font substitution
**XNA behaviour:** `MenuFont` and `HUDFont` use the **"Moire ExtraBold"** TrueType
font (sizes 24 and 18).
**CNA port behaviour:** Generated from **DejaVuSans-Bold** at the same sizes (CNA has
no `.spritefont` pipeline; atlases are built with `tools/make_font.py`). Glyph
metrics differ slightly, so text widths are not pixel-identical to the original.
**Root cause:** "Moire ExtraBold" is not available; CNA needs a bitmap-font atlas.
**Tracked in:** DEFERRED.md item 2 (SpriteFont pipeline).

## No isolated-storage state serialization
**XNA behaviour:** `ScreenManager` can serialize/deserialize the screen stack to
isolated storage (tombstoning support).
**CNA port behaviour:** Omitted; the desktop port has no tombstoning lifecycle.
**Root cause:** Windows Phone application-lifecycle feature.
**Tracked in:** not planned.
