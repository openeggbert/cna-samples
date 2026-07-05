# Missing / Differences from XNA 4.0 original

## Windows Phone / Xbox conditional code dropped
**XNA behaviour:** `#if WINDOWS_PHONE` (touch gestures, `TouchPanel.EnabledGestures`)
and `#if XBOX` (gamepad-only menu navigation, `InputHelper` on-screen cursor as
the primary input) branches exist throughout `ScreenManager`, `MenuScreen`,
`Button.cs`, `BetGameComponent.cs`.
**CNA port behaviour:** Only the `#if WINDOWS` / desktop branch is ported —
real mouse (`Mouse::GetState()`) and keyboard input, matching every other
ScreenManager port in this repo. `InputHelper` (the Xbox gamepad cursor) is
still ported and instantiated (`Button`/`BetGameComponent` look it up
unconditionally even off Xbox in the original), but stays inert/hidden.
**Root cause:** Desktop-only target; no Xbox/Windows Phone equivalent in CNA.
**Tracked in:** Same precedent as every prior ScreenManager port in this repo.

## `Player.Hand` renamed to `PlayerHand`; `CardsGame.Game` renamed to `GameInstance`
**XNA behaviour:** `Player.Hand` and `CardsGame.Game` are ordinary C# property
names; C# has no ambiguity between a property named `Hand`/`Game` and a type
of the same name.
**CNA port behaviour:** Renamed to `Player::PlayerHand` and
`CardsGame::GameInstance` throughout the port. `Hand` and
`Microsoft::Xna::Framework::Game` are both real C++ type names in this
codebase; a member literally named the same as its own type (or an unrelated
type in scope) risks lookup ambiguity in some contexts.
**Root cause:** C++ naming constraint, not a behavioral difference.
**Tracked in:** N/A — mechanical rename, applied consistently everywhere both
were referenced.

## Component ownership: `AddComponent<T>()`/`RemoveComponent()` replace GC + `Dispose`
**XNA behaviour:** `Game.Components` holds GC-reachable references. Removing
a component (`Game.Components.Remove(x)`) just drops the collection's
reference; the GC frees it whenever, and `AnimatedHandGameComponent`
additionally subscribes to `Game.Components.ComponentRemoved` to know when to
unsubscribe from its `Hand`'s events (via `Dispose()`).
**CNA port behaviour:** CNA's `GameComponentCollection` stores raw
non-owning `IGameComponent*`, like real XNA's own `List<IGameComponent>`
does at the framework level -- there's no GC providing implicit ownership.
`CardsFramework::CardsGame` adds `AddComponent<T>(...)` (constructs a
`shared_ptr<T>`, adds the raw pointer to `Game.Components`, and keeps the
`shared_ptr` alive in an internal list) and `RemoveComponent()`/
`RemoveComponentByRaw()` (removes from `Game.Components` immediately, but
defers the actual `shared_ptr` release to `FlushPendingReleases()`, called at
the top of `BlackjackCardGame::Update()` each frame). This mirrors the
"defer destruction to the start of the next frame" pattern already
established for HoneycombRush/NinjAcademy's `ScreenManager` (see NEXT.md's
"pure virtual method called" pattern-to-watch-for) -- destroying a component
while `Game::Update()`/`Draw()` is mid-iteration over its `Components`
snapshot is undefined behavior. `AnimatedHandGameComponent` unsubscribes from
its `Hand`'s events in its own destructor instead of porting the
`ComponentRemoved`+`Dispose()` event dance -- C++'s destructor already runs
deterministically once the last `shared_ptr` reference drops, achieving the
same effect via the language's own idiom.
**Root cause:** No garbage collector in C++; `Game.Components` needs an
explicit owner.
**Tracked in:** Same class of adaptation as HoneycombRush/NinjAcademy's
`pendingDestruction_`/deferred-release pattern.

## `TraditionalCard` identity preserved via `unique_ptr`-per-card, not value copies
**XNA behaviour:** `TraditionalCard` is a reference type; the same object
instance keeps its identity as it moves between `CardPacket`/`Hand`
collections (`AnimatedHandGameComponent.GetCardLocationInHand` compares by
reference equality).
**CNA port behaviour:** `CardPacket`/`Hand` store
`std::vector<std::unique_ptr<TraditionalCard>>` and *move* the `unique_ptr`
between packets/hands (`TraditionalCard::MoveToHand`) rather than copying the
`TraditionalCard` value, so pointer identity survives shuffles, deals, and
splits exactly like the original's reference semantics.
**Root cause:** C++ has no reference types with GC-managed identity by
default; this is the natural C++ equivalent.
**Tracked in:** N/A -- structural adaptation, not a behavioral difference.

## `EventHandler<T>::Raise()` sender argument: `this` replaced with `nullptr`
**XNA behaviour:** C# events pass the raising object as `sender` (`object`);
every type can be a sender since everything derives `object`.
**CNA port behaviour:** `System::EventHandler<T>::Raise()` requires a
`System::Object*` sender. `CardsFramework::CardPacket`, `Hand`, `GameRule`,
and `Blackjack::BlackjackAIPlayer` (via `BlackjackPlayer`/`Player`) don't
derive `System::Object`, so their `Raise(this, ...)` calls pass `nullptr`
instead. None of this port's subscriber lambdas read the `sender` parameter,
so this has no observable effect. (`Blackjack::Button::FireClick()` still
passes `this` correctly -- `Button` derives `AnimatedGameComponent` →
`DrawableGameComponent` → `GameComponent` → `System::Object`.)
**Root cause:** C++ has no universal object root; only CNA's own
component/GameComponent hierarchy derives `System::Object`.
**Tracked in:** N/A -- mechanical fix, no behavioral difference.

## Asset path casing fixed for two files (Linux is case-sensitive)
**XNA behaviour:** `BlackjackCardGame.EndGame()` loads `"Images\youlose"`
(lowercase); `GameplayScreen`/`BlackjackCardGame` request
`"shuffle_" + theme` (lowercase `s`). Windows' content pipeline and
filesystem are case-insensitive, so these resolve fine there even though the
shipped assets are `youLose.png` (capital L) and `Shuffle_Red.png`/
`Shuffle_Blue.png` (capital S).
**CNA port behaviour:** Requests `"Images/youLose"` and `"Shuffle_" + theme`
(matching the real file names) at both the load site and the lookup site.
**Root cause:** CNA runs on a case-sensitive Linux filesystem; a literal port
of the original strings would fail to load these two assets.
**Tracked in:** Same class of fix as other CNA-samples ports that hit
Windows/Linux filesystem-casing mismatches.

## `GetResultAsset` quirk faithfully reproduced, not "fixed"
**XNA behaviour:** `BlackjackCardGame.GetResultAsset(player, dealerValue,
playerValue)` checks `player.BlackJack` (the player's *first*-hand blackjack
flag) to decide push-vs-lose against a dealer blackjack, even when it's being
called to render the result of the player's *second* (split) hand -- it
never consults `player.SecondBlackJack` for this specific decision, unlike
the blackjack/bust booleans `ShowResultForPlayer` computes just before
calling it.
**CNA port behaviour:** Reproduced exactly (`GetResultAsset` takes the
`BlackjackPlayer*` and reads `player->BlackJack` unconditionally), per this
project's "stay as close as possible to the original" philosophy.
**Root cause:** N/A -- original behavior, not introduced by this port.
**Tracked in:** N/A; documented here so it isn't mistaken for a porting bug.

## `AudioManager.LoadMusic()`/`PlayMusic()` are unreachable, matching the original
**XNA behaviour:** `AudioManager` supports background music
(`LoadMusic()`/`PlayMusic()`/`StopMusic()`), but `BlackjackGame.LoadContent()`
only ever calls `AudioManager.LoadSounds()` -- `LoadMusic()` is never called.
No `InGameSong_Loop`/`MenuMusic_Loop` audio assets ship in
`BlackjackHiDefContent/Sounds/` either (only `Bet`/`CardFlip`/
`CardsShuffle`/`Deal`).
**CNA port behaviour:** Ported faithfully, including the same dead code path
-- `LoadMusic()`/`PlayMusic()` exist but are never invoked.
**Root cause:** N/A -- dead code in the original sample itself.
**Tracked in:** N/A; documented so it isn't mistaken for an incomplete port.

## `InstructionScreen` ported but unreachable, matching the original
**XNA behaviour:** `Screens/InstructionScreen.cs` exists but nothing in the
original sample ever constructs one -- `MainMenuScreen`/`GameplayScreen`/
`PauseScreen` never reference it.
**CNA port behaviour:** Ported faithfully (`Screens/InstructionScreen.hpp`,
compiled as part of the build) but equally never instantiated.
**Root cause:** N/A -- dead code in the original sample itself.
**Tracked in:** N/A.

## `CardPacket`'s parameterless `Remove()` (remove-all) omitted
**XNA behaviour:** `CardPacket.Remove()` (no arguments) removes and returns
every card in the packet. No caller in the framework or the Blackjack game
ever calls this overload.
**CNA port behaviour:** Not ported -- confirmed unused via a full-repo grep
of the original source before omitting it.
**Root cause:** N/A -- avoids porting genuinely dead API surface.
**Tracked in:** N/A.

## `MenuScreen`'s Xbox/Windows-Phone-only hit-testing helpers dropped
**XNA behaviour:** `MenuScreen.GetMenuEntryHitBounds()`/`menuEntryPadding`
exist but are only used by the `#elif XBOX`/`#elif WINDOWS_PHONE` branches of
`HandleInput()` -- the `#if WINDOWS` branch this port follows tests
`MenuEntry.Destination.Contains(...)` directly and never calls
`GetMenuEntryHitBounds()`. `MenuScreen.UpdateMenuEntryLocations()` (vertical,
centered layout) is also present but dead: its body's only line
(`menuEntry.Position = position;`) is commented out in the original, and
`Draw()`'s call to it is commented out too -- this sample completely replaced
it with `UpdateMenuEntryDestination()` (horizontal row layout) without
cleaning up the leftover.
**CNA port behaviour:** Neither is ported -- both are genuinely dead code
for the desktop/mouse path this port targets.
**Root cause:** N/A -- avoids porting unreachable/no-op original code.
**Tracked in:** N/A.

## `ScreenManager.SerializeState()`/`DeserializeState()` (IsolatedStorageFile) dropped
**XNA behaviour:** The stock template supports serializing the screen stack
to `IsolatedStorageFile`, but `BlackjackGame`/`GameplayScreen`/`PauseScreen`
never call either method -- there is no save/resume feature in this sample
at all (player balance resets to $500 every launch, same as the original).
**CNA port behaviour:** Not ported -- CNA has no `IsolatedStorageFile`
equivalent, and the methods are unreachable in this sample regardless.
**Root cause:** N/A -- dead code in the original sample itself.
**Tracked in:** Same precedent as every other ScreenManager port in this repo.

## Verification: idle main menu confirmed; deliberate click-through not completed
**What was checked:** Built and ran `CardsStarterKit_cna_samples` under
`SDL_VIDEODRIVER=x11` twice. A clean idle screenshot (no synthetic input sent
before capture) confirms the "BLACKJACK" main menu renders correctly:
title art, Play/Theme/Exit buttons in a horizontal row over the table
background, betting rings visible.
**What was not confirmed:** An earlier same-session run showed the
Deal/Clear betting screen already active within ~2 seconds of launch with no
deliberate click from this agent -- almost certainly stray input reaching the
window (see the `feedback_xdotool_shared_desktop` memory gotcha). Before
attempting a deliberate "click Play" test, `xdotool getactivewindow` showed a
real user window (`gitk`) holding actual focus, not the game window --
per that gotcha's guidance, synthetic input was stopped immediately rather
than risking interference with the user's own session. So: build success and
idle-render are confirmed; a controlled Play → bet → deal → hit/stand
playthrough is **not** confirmed this session and is owed next time input can
be safely driven.
