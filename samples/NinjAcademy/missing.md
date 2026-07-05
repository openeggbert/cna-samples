# Missing / Differences from XNA 4.0 original

## Windows Phone tombstoning dropped
**XNA behaviour:** `NinjAcademyGame` hooks `PhoneApplicationService.Activated/Deactivated/Launching`
and serializes `GameState` (score, hit points, phase, elapsed phase time) to
`IsolatedStorageFile` so the game can resume mid-round after the OS
tombstones and reactivates the app.
**CNA port behaviour:** Always starts fresh at the main menu; `StartSelected()`
always takes the "no saved game" branch.
**Root cause:** No Windows Phone application-lifecycle equivalent exists on
desktop; CNA has no tombstoning concept.
**Tracked in:** Same class of deviation as this project's other ScreenManager
ports (GameStateManagement, HoneycombRush, UISample) — established precedent,
not a new gap.

## High-score name entry uses a custom keyboard popup instead of Guide
**XNA behaviour:** `Guide.BeginShowKeyboardInput` shows the OS on-screen
keyboard and returns the typed name asynchronously.
**CNA port behaviour:** CNA's `Guide::BeginShowKeyboardInput`/`EndShowKeyboardInput`
always completes with an empty string (no system keyboard on this platform),
and `Guide::BeginShowMessageBox` always throws — so using `Guide` directly was
not viable. `Screens/HighScoreScreen.hpp` adds `NameEntryScreen`, a small
popup that reads real keyboard input (A-Z, Backspace, Enter) via CNA's actual
`Keyboard` API and then calls `HighScoreScreen::PutHighScore()` — a genuine
substitute feature, not a stub returning a fixed "Player" name.
**Root cause:** CNA's `Guide` is a stub for these two calls (see
`cna/include/Microsoft/Xna/Framework/GamerServices/Guide.hpp`); this is a
real, documented platform limitation, not a bug to fix in CNA.
**Tracked in:** CLAUDE.md "Assets"/"do not work around CNA bugs" — this is a
sample-side feature addition to route around a stub API, not a workaround for
a framework bug.

## High-score persistence uses a plain text file, not IsolatedStorageFile
**XNA behaviour:** `HighScoreScreen` saves/loads the table via
`IsolatedStorageFile`.
**CNA port behaviour:** Uses `std::ofstream`/`std::ifstream` against
`highscores.txt` next to the binary.
**Root cause:** CNA has no `IsolatedStorageFile` equivalent.
**Tracked in:** Same precedent as HoneycombRush's `HighScoreScreen.hpp`.

## Mouse fallback for tap/drag input
**XNA behaviour:** All input is `TouchPanel` gestures (`Tap` for menu
selection/throwing a shuriken, `FreeDrag` for sword slashes) — this is a
Windows Phone game with no keyboard/mouse input path at all.
**CNA port behaviour:** `ScreenManager/InputState.hpp` synthesizes a `Tap`
gesture on a mouse left-click rising edge, and a `FreeDrag` gesture each frame
the button stays held, matching this project's established "every
interaction is a discrete tap/drag" mouse fallback (DynamicMenu/UISample
precedent, pattern 3/4 in NEXT.md section 6).
**Root cause:** This desktop has no touchscreen and CNA does not synthesize
touch/gesture events from mouse input.
**Tracked in:** NEXT.md section 6.

## Background-thread asset loading simplified to synchronous
**XNA behaviour:** `LoadingScreen` starts a `System.Threading.Thread` running
`GameplayScreen.LoadAssets()` in the background while showing a "loading"
texture, polling `Thread.ThreadState` each frame.
**CNA port behaviour:** `LoadingScreen::LoadResources()` calls
`gameplayScreen_->LoadAssets()` synchronously in the same frame it's
triggered, then immediately marks loading as finished. This sample's asset
set is small and loads well within one frame on desktop hardware, so the
busy-texture screen is only ever visible for at most one frame.
**Root cause:** No concrete benefit to a background thread on desktop for
this sample's small asset set; avoids adding thread-safety concerns to
GameContent loading, which is not documented as thread-safe in CNA.
**Tracked in:** Same precedent as HoneycombRush's `GameplayScreen::LoadAssets()`.

## Animation/Configuration XML hand-translated to C++ construction code
**XNA behaviour:** `Textures/Animations.xml` and `Configuration/Configuration.xml`
are parsed at content-build time by `NinjAcademyPipeline/AnimationProcessor.cs`
and `ConfigurationProcessor.cs` into `AnimationStore`/`GameConfiguration`
objects, then loaded via `Content.Load<AnimationStore>(...)` /
`Content.Load<GameConfiguration>(...)`.
**CNA port behaviour:** `AnimationStore.hpp`'s `BuildAnimationStore()` and
`GameConfiguration.hpp`'s `BuildConfiguration()` hand-translate the exact
values from both XML files into C++ construction code, called directly
instead of going through `ContentManager::Load`.
**Root cause:** CNA has no general XML content-pipeline deserializer.
**Tracked in:** Same precedent as DynamicMenu's `MenuPage2.xml`.

## Faithful reproduction of two original timing quirks (not "fixed")
**XNA behaviour (Animation.cs):** `Animation::Update()` only advances a frame
when `frameChangeTimer >= frameChangeInterval`, but `frameChangeTimer` is only
ever reset to `TimeSpan.Zero` — it is never incremented by elapsed game time
anywhere in the original `Animation.cs`. In practice this means animations
(gold target spin, dynamite fuse, explosion) rarely advance past their first
frame transition in the shipped original.
**XNA behaviour (GameplayScreen.cs):** `ManageGamePhase()` adds
`gameTime.ElapsedGameTime` to `upperTargetTimer`/`middleTargetTimer`/
`lowerTargetTimer` *and then* `ManagePhaseTargets()` adds it again internally
— targets appear on a roughly 2x-faster cadence than `Configuration.xml`'s
`Interval` attributes specify.
**CNA port behaviour:** Both quirks are reproduced exactly as in the original
(`Animation::Update()` in `Animation.hpp`; the double-increment in
`GameplayScreen::ManageGamePhase()`/`ManagePhaseTargets()`), per this
project's "stay as close as possible to the original" philosophy — not
treated as bugs to fix.
**Root cause:** N/A — this is the original's own behavior.
**Tracked in:** N/A; documented here so it isn't mistaken for a porting bug.

## `TextDisplayComponent` ownership kept in a vector, not a single field
**XNA behaviour:** `GameplayScreen.MarkGameOver()` adds a new
`TextDisplayComponent` (the "Game Over" text) to `Game.Components` each time
it's called; the C# GC keeps it alive for as long as `Game.Components`
references it.
**CNA port behaviour:** `GameplayScreen` accumulates every created
`TextDisplayComponent` in a `std::vector<std::shared_ptr<TextDisplayComponent>>`
rather than overwriting a single field, since `Game.Components` only stores a
raw pointer with no ownership — overwriting the single owning `shared_ptr`
would destroy the object while a dangling raw pointer stayed registered.
**Root cause:** C++ has no GC; `Game.Components` needs an explicit owner.
**Tracked in:** Same class of adaptation as the `pendingDestruction_`
shared_ptr-lifetime pattern documented in NEXT.md's "pattern to watch for".

## No known CNA framework gaps hit
Nothing in this port required a change to `cna` or `sharp-runtime`. Every
adaptation above is either an established project precedent or a
sample-local addition (NameEntryScreen) built entirely from existing, real
CNA APIs (`Keyboard`, `TouchPanel`, `SpriteBatch`, `Guide` only where its stub
behavior didn't matter).
