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

## Fullscreen omitted (fixed 30 fps timestep kept faithfully)
**XNA behaviour:** The constructor sets `graphics.IsFullScreen = true` unconditionally
("Switch to full screen for best game experience") — on Windows Phone this just fills
the screen; on a desktop Windows build it would force an actual fullscreen window. It
also sets `TargetElapsedTime = TimeSpan.FromTicks(333333)` (30 fps); the 30 fps timestep
matters because catapult/projectile animations advance exactly one frame per `Update()`
call.
**CNA port behaviour:** Left windowed, matching every other sample in this repo —
forcing fullscreen would make screenshotting/testing this one sample inconsistent
with the rest of the project for no behavioral benefit on desktop. Unlike most other
samples in this repo, though, the 30 fps fixed timestep itself **is** kept
(`setTargetElapsedTimeProperty(TimeSpan::FromSeconds(1.0/30.0))`, with an explanatory
comment in `CatapultGame.hpp`), so animation speed matches the original exactly.
**Root cause:** Desktop dev-loop practicality for the fullscreen bit; the 30 fps
timestep is a deliberate exception to the repo's usual "omit phone timestep, default
to 60 fps" pattern (see e.g. Bounce/PathDrawing's missing.md) because this sample's
animations are frame-stepped rather than time-scaled.
**Tracked in:** not planned.

## Case-sensitive asset path: drag-arrow texture
**XNA behaviour:** `Human.Initialize()` loads
`Content.Load<Texture2D>("Textures/HUD/arrow")` (lower-case `arrow`) — resolves
fine under Windows' case-insensitive filesystem/content pipeline.
**CNA port behaviour:** The shipped PNG is `Content/Textures/HUD/Arrow.png`
(capital `A`), so `Human::Initialize()` loads `"Textures/HUD/Arrow"` to match
the file actually on disk (see the inline comment in `Human.hpp`).
**Root cause:** Linux's filesystem is case-sensitive; same class of fix as
documented in RolePlayingGame/HoneycombRush/CardsStarterKit's missing.md.
**Tracked in:** not planned.

## AudioManager: `GameComponent` singleton → static utility class
**XNA behaviour:** `AudioManager` derives from `GameComponent` and
`AudioManager.Initialize(game)` calls `game.Components.Add(audioManager)`,
registering it in the `Game.Components` collection (mainly for automatic
`Dispose()` cleanup on exit; `Update`/`Draw` are never overridden).
**CNA port behaviour:** `AudioManager` is a plain static utility class holding
`inline static` sound-effect state; `AudioManager::Initialize(Game*)` only
stashes the `Game*` pointer and never registers with
`getComponentsProperty()`. All public methods (`PlaySound`, `StopSounds`,
`PauseResumeSounds`, `PlayMusic`, ...) behave identically; only component-list
membership and the C# `Dispose` cleanup path are dropped.
**Root cause:** No behavioural need for `GameComponent` membership since the
original never used its `Update`/`Draw` hooks; a static utility is simpler in
C++ than a component that exists solely to be found via the singleton pattern.
**Tracked in:** not planned.

## No isolated-storage state serialization
**XNA behaviour:** `ScreenManager` can serialize/deserialize the screen stack to
isolated storage (tombstoning support).
**CNA port behaviour:** Omitted; the desktop port has no tombstoning lifecycle.
**Root cause:** Windows Phone application-lifecycle feature.
**Tracked in:** not planned.
