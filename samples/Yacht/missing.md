# Missing / Differences from XNA 4.0 original

The original is the **Yacht Starter Kit**
(`/rv/tmp/XNAGameStudio/Samples/Yacht_4_0/`), a Windows Phone 7 dice game with
an optional WCF-based online multiplayer mode.

## Input: real touch + real accelerometer (not remapped to mouse only)
**XNA behaviour:** Driven by `TouchPanel` gestures (Tap/VerticalDrag/DragComplete)
for menus, dice, buttons and the scorecard, plus the phone's accelerometer for
shake-to-roll.
**CNA port behaviour:** Unlike the earlier CatapultWars/GameStateManagement
ports, this sample uses CNA's **real** `TouchPanel`/`GestureDetector` (SDL3
finger events) and CNA's **real** `Microsoft::Devices::Sensors::Accelerometer`
(SDL3 `SDL_Sensor`) — both are fully implemented in CNA, an earlier `NEXT.md`
note calling touch "not supported" was stale. Mouse is added as an explicit
parallel input path everywhere the original only had touch (Roll/Score
buttons, dice pick-up, scorecard tap/scroll, game-over restart); menu
navigation already had keyboard/gamepad/mouse from the shared ScreenManager
pattern. On this development desktop there is no physical accelerometer
(`getIsSupportedProperty()` is false), so the practical, verifiable path here
is the original's *own* keyboard arrow-key emulation fallback (`Accelerometer.cs`
already had this for non-device/emulator testing) — real hardware shake is
implemented and should work unmodified on a machine with a real sensor, but
was not verified on this desktop.
**Root cause:** N/A — this is a genuine capability, not a workaround.
**Tracked in:** not planned; `NEXT.md`'s stale "No touch input" line was corrected.

## One behavioural fix: shake-to-roll is reachable on desktop
**XNA behaviour:** `Accelerometer.GetState()` (the emulator/keyboard-fallback
path) is never actually called anywhere in the shipped Yacht sample — only the
real-hardware `ReadingChanged` event handler runs `CheckForShake`. This means
shake-to-roll was **effectively dead code** when the original sample ran in
the Windows Phone emulator (no physical accelerometer), since nothing ever
polled `GetState()` there.
**CNA port behaviour:** `HumanPlayer` polls `Accelerometer::GetState()` once
per frame (running the same `CheckForShake` algorithm either way), so the
keyboard-fallback shake gesture actually works and is verifiable on a desktop
with no accelerometer.
**Root cause:** Deliberate fix so the feature is testable here, per the
approved port plan.
**Tracked in:** not planned.

## Online multiplayer removed entirely
**XNA behaviour:** An "Online Game" main-menu entry connects to a WCF service
(`YachtServices`) over HTTP, using Windows Phone push-notification channels
(`Microsoft.Phone.Notification`) to notify players of opponent turns.
**CNA port behaviour:** Dropped completely — `Misc/NetworkManager.cs`,
`Screens/SelectOnlineGameScreen.cs`, `Objects/NetworkPlayer.cs`, and the
"Online Game" menu entry have no C++ counterpart. Only local Human-vs-AI play
(the original's own "Offline Game" mode) is ported; it was confirmed
self-contained with no dependency on the network layer.
**Root cause:** Windows Phone server/push infrastructure has no CNA or
general-desktop equivalent, and porting a WCF service to C++ is out of scope
for a samples repo.
**Tracked in:** not planned.

## Save/load (tombstoning) removed; screen flow simplified accordingly
**XNA behaviour:** `YachtGame` saves/loads game state to isolated storage via
`PhoneApplicationService` Activated/Deactivated/Closing/Launching handlers
(Windows Phone's tombstoning lifecycle), and `NewGameSubMenuScreen` offers a
"Load" entry alongside "New Game" when a saved game exists.
**CNA port behaviour:** Dropped entirely — no tombstoning lifecycle exists on
desktop. `NewGameSubMenuScreen` keeps only its "New Game" entry. "Offline
Game" on the main menu now always leads to `NewGameSubMenuScreen` (in the
original, it would sometimes skip straight past this screen if no save file
existed), which keeps the screen structure faithful rather than making it
silently unreachable.
**Root cause:** Windows Phone application-lifecycle feature with no CNA
desktop equivalent.
**Tracked in:** not planned.

## Player name entry simplified
**XNA behaviour:** `Guide.BeginShowKeyboardInput` shows an on-screen keyboard
to let the player type their name before starting an offline game.
**CNA port behaviour:** The human player's name is a fixed `"Player1"` literal
(the same placeholder string the original itself already used on its online
path) — no on-screen keyboard exists in CNA and no other sample has one.
**Root cause:** No CNA/desktop equivalent to `Guide.BeginShowKeyboardInput`.
**Tracked in:** not planned.

## Dice roll animation: background Timer → per-frame accumulator
**XNA behaviour:** `Dice.cs` uses a shared `static Random` plus a self-rearming
`System.Threading.Timer` running on a background thread to animate the
rolling face (~300-600ms per re-roll, ~80% chance to keep spinning each tick).
**CNA port behaviour:** Same odds and timings, but driven by a per-die
elapsed-time accumulator inside `Dice::Update(GameTime&)`, called every frame
from the main `Update` loop — no background thread. `random_` stays an
`inline static System::Random` shared across all dice, matching the
original's shared RNG. The AI's inter-turn "thinking" delay (also a
`System.Threading.Timer` in the original) is ported the same way.
**Root cause:** No background-thread timer primitive is used anywhere else in
this repo; an accumulator is the established idiom for frame-driven timing.
**Tracked in:** not planned.

## Font substitution
**XNA behaviour:** Five `.spritefont` files use **Impact** (MenuFont 22,
LeaderScoreFont 22 Italic), **Segoe UI Mono** (Regular 20), and **Lindsey**
(ScoreFont/ScoreFontBold 18).
**CNA port behaviour:** Generated from DejaVu substitutes at the same sizes
via `tools/make_font.py` (CNA has no `.spritefont`/TTF-at-runtime pipeline):
DejaVuSans-Bold → MenuFont, DejaVuSans-BoldOblique → LeaderScoreFont,
DejaVuSansMono → Regular, DejaVuSans-Bold → ScoreFontBold, DejaVuSans →
ScoreFont. Glyph metrics differ slightly from the originals.
**Root cause:** Impact/Segoe UI Mono/Lindsey are not available; CNA needs a
bitmap-font atlas.
**Tracked in:** DEFERRED.md item 2 (SpriteFont pipeline) — already ✅ resolved
for the atlas mechanism itself; only the specific font files are substituted.

## Unused/online-only art dropped
**CNA port behaviour:** 11 of the 28 files under `YachtContent/Images/` are not
copied to `Content/Images/`, verified by grepping every reference in the C#
source (case-insensitively, covering both `@"Images\Foo"` and `"Images/Foo"`
string styles used inconsistently across the original):
- Online-only (referenced only by the dropped `NetworkManager`/
  `SelectOnlineGameScreen`): `Connect.png`, `Search.png`,
  `online_game_selectionBG.png`.
- Confirmed **zero references anywhere** in the original source (dead art
  independent of the online/offline split): `New.png`, `star.png`, `star.jpg`,
  `selectOnlineGamebackground.png`, `line.png`, `scorecard.png`,
  `ArrowUp.png`, `ArrowDown.png`. `Yacht/Background.png` (outside
  `YachtContent/`, next to `YachtGame.cs`) is also unreferenced and was not
  copied.
- `button.png` and `Dot.png` (lowercase `dot` in the original's own
  `@"Images\dot"` string) are **kept** — an initial pass wrongly flagged them
  as online-only/dead due to a grep bug (missed forward-slash string style and
  case), but both are genuinely loaded by offline-relevant code:
  `ScreenManager.cs`'s own shared fade/blank transition texture
  (`"Images/button"`), and `GameStateHandler.cs`'s current-player star icon on
  the leaderboard (`@"Images\dot"`).
**Root cause:** N/A — these assets are either online-only or were already
unused in the shipped original.
**Tracked in:** not planned.

## No isolated-storage state serialization
**XNA behaviour:** Game state (scorecard, current turn, dice) can be
serialized to isolated storage as part of the tombstoning lifecycle.
**CNA port behaviour:** Omitted — see "Save/load (tombstoning) removed" above.
**Root cause:** Windows Phone application-lifecycle feature.
**Tracked in:** not planned.
