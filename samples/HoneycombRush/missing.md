# Missing / Differences from XNA 4.0 original

## ScreenManager: deferred screen destruction (real C++ port bug, fixed)
**XNA behaviour:** `ScreenManager.RemoveScreen()` just removes a screen reference
from the `screens` list; the .NET garbage collector frees the underlying
`GameScreen` (and, for `GameplayScreen`, the dozens of `Beehive`/`Bee`/`ScoreBar`/etc.
objects it owns) whenever it gets around to it — never synchronously, and never in
the middle of `Game.Update()`/`Game.Draw()` iterating `Game.Components`.
**CNA port behaviour:** `GameplayScreen` has `TransitionOffTime = 0`, so leaving it
(e.g. tapping "Exit" on the in-game pause menu) makes `GameScreen::ExitScreen()` call
`ScreenManager::RemoveScreen()` **synchronously**, inside the same `Game::Update()`
call that is iterating a *snapshot* of `Game.Components` taken at the top of the
frame. `RemoveScreen()`'s `UnloadContent()` call removes ~100 `Bee`/`Beehive`/
`ScoreBar`/etc. component pointers from `Game.Components` (safe — CNA's own
add/remove-event wiring keeps `Game`'s internal update/draw lists in sync). But once
the caller's last `shared_ptr<GameScreen>` reference to `GameplayScreen` goes out of
scope (at the end of `PauseScreen::OnCancel()`'s screen-exiting loop), `GameplayScreen`
gets destructed **immediately**, cascading into the destruction of every `Bee`/
`Beehive`/`ScoreBar`/etc. it owns — while `Game::Update()`'s *already-taken* snapshot
for this same frame still holds now-dangling raw pointers to some of those objects
(only elements that hadn't been visited by the iterator yet — the crash is
nondeterministic-looking because it depends on component update order). The next loop
iteration calls a virtual method through a freed vtable pointer, producing
`pure virtual method called` and an immediate `terminate()`/`SIGABRT`. Reliably
reproduced (2/2) by: Start → tap loading screen → wait for gameplay → Escape (open
pause menu) → tap "Exit".
**Fix:** `ScreenManager` now defers the actual `shared_ptr` release: `RemoveScreen()`
moves the screen's `shared_ptr` into a `pendingDestruction_` member instead of letting
it drop, and `ScreenManager::Update()` clears `pendingDestruction_` (destroying
whatever was queued last frame) as its very first action — a point guaranteed to be
outside any `Game::Update()`/`Draw()` component-iteration snapshot, since the previous
frame's Update *and* Draw have both already fully completed by then. This is a
C++/RAII-specific hazard with no equivalent in the GC-based original; the fix lives
entirely in this sample's own `ScreenManager.hpp` (no CNA changes).
**Root cause:** deterministic `shared_ptr` destruction synchronously freeing objects
that a same-frame `Game::Component` iteration snapshot still references, vs. XNA's
GC never doing so mid-iteration.
**Tracked in:** not planned — fixed in this port's `ScreenManager.hpp`.

## AudioManager: dangling `SoundEffect*` inside `SoundEffectInstance` (real C++ port bug, fixed)
**XNA behaviour:** `SoundEffectInstance` holds a managed reference to its source
`SoundEffect`; a local `SoundEffect soundEffect = Content.Load<SoundEffect>(...)`
going out of scope at the end of `LoadSound()` doesn't matter — the GC keeps the
`SoundEffect` alive for as long as any `SoundEffectInstance` (or anything else)
references it.
**CNA port behaviour:** CNA's `SoundEffectInstance` stores a raw `const SoundEffect*`
back to the `SoundEffect` it was created from (`soundEffect_`, dereferenced by
`Play()` to reach the native audio handle). The original translation of
`AudioManager.LoadSound()` created the `SoundEffect` as a local stack variable,
created a `SoundEffectInstance` from it, and stored only the *instance* — the
`SoundEffect` itself was destroyed the instant `LoadSound()` returned, leaving every
cached `SoundEffectInstance` with a dangling pointer. This was a real, reliably
reproduced segfault: the very first `SoundEffectInstance::Play()` call in the whole
sample (the "Go!" countdown's `BeeBuzzing_Loop` sound) crashed immediately.
**Fix:** `AudioManager` now also caches the `SoundEffect` objects themselves (a
`soundEffects_` map, keyed by alias, populated via `try_emplace` before
`CreateInstance()` is called), keeping them alive for `AudioManager`'s entire
(effectively program) lifetime.
**Root cause:** C++ raw-pointer-to-a-stack-temporary lifetime bug, not present in the
GC-based original.
**Tracked in:** not planned — fixed in this port's `AudioManager.hpp`.

## BeeKeeper: dangling temporary `Texture2D` passed to a deferred `SpriteBatch::Draw` (real C++ port bug, fixed)
**XNA behaviour:** `spriteBatch.Draw(Game.Content.Load<Texture2D>("Textures/hit"), position, Color.White);`
inside `BeeKeeper.Draw()` — reloading the (GC/cache-backed) texture every frame while
stung is wasteful but harmless.
**CNA port behaviour:** `SpriteBatch::Draw()` defers the actual GPU draw call to
`End()` (deferred sprite-sort mode, the default) by storing a raw `Texture2D*`
pointing at its `const Texture2D&` argument. The initial translation passed
`Load<Texture2D>("Textures/hit")` — a temporary — directly into `Draw()`; the
temporary was destroyed at the end of that statement, well before `End()` ran,
leaving `SpriteBatch`'s internal `current_texture_` dangling and crashing on the
first sting.
**Fix:** `BeeKeeper` now loads the "hit" texture once into a member
(`hitTexture_`, in `LoadContent()`) and passes that stable reference to `Draw()`
instead.
**Root cause:** same class of dangling-temporary-passed-to-deferred-rendering bug as
the `AudioManager` one above; a project-wide grep confirmed no other call site does
this.
**Tracked in:** not planned — fixed in this port's `BeeKeeper.hpp`/`GameplayScreen.hpp`.

## `System.Threading.Thread` background loading → synchronous load with a one-frame delay
**XNA behaviour:** `LoadingAndInstructionScreen`/`LevelOverScreen` start a background
`Thread` running `GameplayScreen.LoadAssets()` when tapped, polling
`thread.ThreadState == ThreadState.Stopped` in `Update()` to know when to switch to
the gameplay screen, so the "Loading..." text has a chance to render for at least one
frame while the real work happens off the UI thread.
**CNA port behaviour:** `LoadAssets()` (texture/animation loading and creating ~100
`Bee`/`Beehive`/`ScoreBar`/etc. `GameComponent`s) runs synchronously on the main
thread instead — untested GL-context thread-safety on a background thread was judged
too risky to attempt. To still show the "Loading..." frame for at least one draw
before the (now synchronous, so instant) load actually blocks the frame, a
`loadPending_` flag delays the real `LoadAssets()`/`AddScreen()` call by exactly one
`Update()` tick after the tap is registered.
**Root cause:** untested background-thread GL-context safety in CNA; not attempted.
**Tracked in:** not planned.

## `Guide.BeginShowKeyboardInput` → fixed placeholder name
**XNA behaviour:** On reaching a new high score at Hard difficulty, `Guide` pops up an
on-screen keyboard (`Guide.BeginShowKeyboardInput`) to ask the player for their name,
via an async callback (`AfterPlayerEnterName`).
**CNA port behaviour:** CNA's `Guide` is an explicit stub (`Show()`/`IsTrialMode`
only). The high score is saved directly with the fixed name `"Player"`
(`HighScoreScreen::PutHighScore("Player", Score())`), synchronously, in
`CheckIfCurrentGameFinished()` — the `AfterPlayerEnterName` async-callback path was
dropped entirely since there's nothing async left to call back from.
**Root cause:** CNA's `Guide` (`cna/include/Microsoft/Xna/Framework/GamerServices/Guide.hpp`)
is an intentional stub: `BeginShowKeyboardInput`/`EndShowKeyboardInput` always
complete synchronously with an empty string (no system on-screen keyboard on
this platform) — a real, documented platform limitation, not a CNA bug to fix.
**Tracked in:** not planned — no DEFERRED.md item exists for this (there is no
CNA on-screen-keyboard work planned); see CLAUDE.md's "do not work around CNA
bugs" note and the equivalent `NinjAcademy`/`Yacht` writeups for the same stub.

## `IsolatedStorageFile` → plain file I/O
**XNA behaviour:** `HighScoreScreen.SaveHighscore()`/`LoadHighscores()` persist the
5-entry high score table to `highscores.txt` inside the app's sandboxed
`IsolatedStorageFile.GetUserStoreForApplication()`.
**CNA port behaviour:** Persisted to a plain `highscores.txt` file via `std::ofstream`/
`std::ifstream` in the process's current working directory. CNA has no
`IsolatedStorageFile`-equivalent sandboxed storage API.
**Root cause:** no CNA equivalent to WP7's isolated storage.
**Tracked in:** not planned.

## Case-sensitivity fixes (Linux filesystem)
- The `.contentproj` remaps `honeycombRush_instructions.png` to the asset name
  `"instructions"`, but the code that loads it (`MainMenuScreen.StartGameMenuEntrySelected`)
  requests `"Instructions"` (capital I) — works on Windows' case-insensitive
  filesystem, would silently fail to load on Linux. The port's `Content/Textures/Backgrounds/`
  directory has the asset saved as `Instructions.png` (capitalized) to match the load
  call, rather than changing the load call to match the original lowercase asset name.
- The original itself is internally inconsistent: `HighScoreScreen.Exit()` adds
  `new BackgroundScreen("titlescreen")` (lowercase) while every other call site
  (`HoneycombRush`'s constructor, `PauseScreen.OnCancel`, etc.) uses `"titleScreen"`
  (capital S) — a typo that Windows' case-insensitive filesystem silently papers over.
  The port normalizes every call site to `"titleScreen"` (matching the actual asset
  filename, `Content/Backgrounds/titleScreen.png`) — including the one in
  `HighScoreScreen.hpp` that the original had wrong.
**Root cause:** Linux filesystems are case-sensitive; Windows' are not.
**Tracked in:** not planned — asset renamed / call sites normalized, no further work.

## Keyboard fallback for virtual thumbsticks (no touchscreen)
**XNA behaviour:** `VirtualThumbsticks` tracks up to two simultaneous touches by
finger ID, splitting the screen into left/right halves; `BeeKeeper`'s direction/sprite
logic reads both `LeftThumbstick` (a drag vector) and `LeftThumbstickCenter` (the
touch's anchor point, used for the on-screen joystick widget's hit-test).
**CNA port behaviour:** This desktop has no touchscreen. When no real left-half touch
is active, `VirtualThumbsticks::Update()` synthesizes one from WASD/arrow keys,
anchored at a fixed point (`SetKeyboardAnchor()`, set by `GameplayScreen` every frame
to the on-screen joystick widget's center) — so both `LeftThumbstick` *and*
`LeftThumbstickCenter` behave exactly as if a real touch were happening there, and
`BeeKeeper`'s direction-facing logic (which checks the center, not just the vector)
needed no changes at all. The right "thumbstick" (smoke button) is left touch-only,
since the original already has an independent Space-key fallback for firing smoke.
Also, `GameplayScreen`'s own `lastTouchPosition` field (updated only from raw
`TouchState` iteration in the original, so it would never reflect keyboard input) is
replaced by `VirtualThumbsticks::getLeftPosition()`, which reflects the
keyboard-synthesized position too — this is what `HandleThumbStick()`'s "is the touch
within the joystick's outer boundary" sanity check now reads.
**Root cause:** this desktop has no touchscreen; matches this project's established
touch/mouse(-and-keyboard)-fallback conventions (see NEXT.md section 6).
**Tracked in:** not planned.

## Mouse-synthesized Tap gesture (menu/loading/level-over/high-score screens)
**XNA behaviour:** `TouchPanel.GetState()`/gestures are the only input these screens
read.
**CNA port behaviour:** `InputState::Update()` additionally synthesizes a single
`GestureType.Tap` on a left mouse-button-down rising edge (pattern 3 from this
project's established conventions), so every screen that only checks
`input.Gestures[0].GestureType == Tap` (all of this sample's menu/loading/level-over/
high-score screens) gets mouse support for free.
**Root cause:** this desktop has no touchscreen; CNA does not synthesize touch/gesture
events from mouse input itself.
**Tracked in:** not planned.

## `AnimationsDefinition.xml` / `Configuration.xml` hand-translated instead of parsed at runtime
**XNA behaviour:** `GameplayScreen.LoadAnimationFromXML()` parses
`Content/Textures/AnimationsDefinition.xml` via `System.Xml.Linq.XDocument` at
runtime; `ConfigurationManager.LoadConfiguration()` similarly parses
`Content/Configuration/Configuration.xml`.
**CNA port behaviour:** Both XML files are hand-translated, once, into static C++
tables (`GameplayScreen::LoadAnimationFromXML()` now builds `Animation` objects
directly from a small hardcoded list; `ConfigurationManager`'s
`Easy`/`Medium`/`Hard` configs are a static `unordered_map` built by an
immediately-invoked lambda) — matching this project's established "no general XML
deserializer, hand-translate once" precedent.
**Root cause:** CNA has no general-purpose XML deserializer.
**Tracked in:** not planned.

## `MenuScreen.UpdateMenuEntryLocations()` not ported (dead code in the original)
**XNA behaviour:** `UpdateMenuEntryLocations()` exists but every line of its body is
commented out, and it is never called from anywhere in the original project — every
concrete `MenuScreen` subclass hardcodes fixed pixel positions for its entries
instead.
**CNA port behaviour:** Not ported; every subclass (`MainMenuScreen`, `PauseScreen`)
hardcodes the same fixed positions the original does.
**Root cause:** dead code in the original.
**Tracked in:** not planned.

## Font substitution
**XNA behaviour:** All five `.spritefont` files (`MenuFont` 48, `HighScoreFont` 50,
`GameScreenFont14px`/`16px`/`36px`) use the **"Moire"** TrueType font (Bold for the
three `GameScreenFont*` sizes, Regular for `MenuFont`/`HighScoreFont`) via the XNA
Content Pipeline.
**CNA port behaviour:** Generated from a DejaVu Sans substitute (matching each
font's original Bold/Regular style) at the same point sizes via `tools/make_font.py`
(CNA has no `.spritefont`/TrueType-at-runtime pipeline; atlases are pre-baked PNG +
`.font.json`). Glyph metrics differ slightly from "Moire", so text widths are not
pixel-identical to the original.
**Root cause:** "Moire" is not available on this system; CNA needs a bitmap-font atlas.
**Tracked in:** DEFERRED.md item 2 (SpriteFont pipeline) — already resolved for the
atlas mechanism itself; only the specific font file is substituted.

## No fullscreen forcing
**XNA behaviour:** `graphics.IsFullScreen = true;` in the main `HoneycombRush`
constructor.
**CNA port behaviour:** Not set — matches this project's established DynamicMenu/
UISample precedent of leaving the sample windowed. The back buffer is left at CNA's
default 800×480, matching the original's native WP7 resolution that every hardcoded
pixel position in this port (beehive/UI positions, etc.) assumes.
**Root cause:** established project convention; fullscreen forcing isn't useful in a
desktop test/dev environment.
**Tracked in:** not planned.

## `decimal` → `float` in `ScoreBar`
**XNA behaviour:** `ScoreBar.GetSpaceFromBroder()`/`GetTextureByCurrentValue()` use
C#'s `decimal` type for the fill-percentage math.
**CNA port behaviour:** Uses plain `float` (`ScoreBar::GetSpaceFromBorder()` — also
fixing the original's `GetSpaceFromBroder` typo). No behavioral difference: the value
range (0–100, at most 3 significant digits) never exercises `decimal`'s extra
precision over `float`.
**Root cause:** simplification; C++ has no built-in decimal type, and none is needed
here.
**Tracked in:** not planned.

## `AsyncCallback` → `std::function<void()>`
**XNA behaviour:** `BeeKeeper.StartTransferHoney(int, AsyncCallback callback)` stores
an `AsyncCallback` delegate, invoked with a `null` `IAsyncResult` once the honey
deposit animation finishes — used as a plain completion callback, not for any real
asynchronous I/O.
**CNA port behaviour:** `BeeKeeper::StartTransferHoney(int, std::function<void()>)` —
a direct, synchronous completion-callback translation; `GameplayScreen::EndHoneyDeposit()`
no longer takes an unused `IAsyncResult`-equivalent parameter.
**Root cause:** `AsyncCallback` was misused as a plain callback in the original; no
real async machinery to preserve.
**Tracked in:** not planned.

## Minor: unused public accessor methods left on some `Objects/*.hpp` classes
A few public getters (e.g. `Beehive::ScoreBarRef()`/`LastTimeHoneyAdded()`,
`HoneyJar::Font16px()`/`HoneyTextSize()`, several on `Vat`) were added anticipating
that out-of-line method definitions in `GameplayScreen.hpp` would need them, but
out-of-line member-function definitions (`ClassName::Method(...)`) already have full
access to private members regardless of where they're textually defined — these
accessors are unnecessary and unused by the final `GameplayScreen.hpp`. Left in place
as harmless dead code rather than spent time reverting given the scope of the port;
safe to remove in a future cleanup pass.
**Tracked in:** not planned — minor cleanup debt, not a behavioral difference.

## Interactive verification
Confirmed by screenshot and direct interaction (`xdotool`): the title screen, "Start"
→ Instructions background → tap-to-load screen → 3-second countdown → "Go!" →
full gameplay (5 beehives with swarming worker/soldier bees, honey jar UI with fill
bar, beekeeper avatar responding to WASD/arrow-key movement via the keyboard-thumbstick
fallback, smoke-gun firing via Space with a visible smoke-puff cloud, the vat's
fill/timer UI). Also confirmed: Escape opens the in-game pause menu ("Resume"/"Exit"),
and tapping "Exit" cleanly returns to the main menu without crashing (see the
`ScreenManager` deferred-destruction fix above — this exact interaction reliably
crashed before the fix). The F1 help overlay was generated from a hand-authored
`HoneycombRushSample.htm` (the original kit ships only a 127-page Word document,
`Honeycomb_Rush.doc`, not an `.htm` file).
