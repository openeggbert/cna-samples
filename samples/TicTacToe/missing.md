# Missing / Differences from XNA 4.0 original

## Networking and WCF web service removed
**XNA behaviour:** The original is a networked Windows Phone 7 game.  All game logic
(move validation, win detection, AI moves) is delegated to a WCF web service
(`TicTacToeService`) running on a separate server.  The client sends moves and receives
game state updates via asynchronous WCF calls and Windows Phone push notifications
(`HttpNotificationChannel`).
**CNA port behaviour:** All game logic runs locally.  No networking.  Win detection
and random-AI moves are implemented client-side.
**Root cause:** Windows Phone APIs (`Microsoft.Phone.Notification`, WCF client proxies,
`IsolatedStorage`) are not available on desktop CNA.
**Tracked in:** not planned (phone-specific feature)

## Touch input replaced with mouse input
**XNA behaviour:** The original uses `TouchPanel.IsGestureAvailable` / `ReadGesture()`
with `GestureType.Tap` to detect where the player taps the board.
**CNA port behaviour:** Left mouse-button click on a board square is used instead.
**Root cause:** `TouchPanel` / touch gestures are phone-specific; not in CNA.
**Tracked in:** not planned (platform adaptation)

## Button widget replaced with keyboard shortcut
**XNA behaviour:** Three touch-operated `Button` widgets appear during gameplay:
"Send Move" (confirms a selected square), "New Game", and "Exit".
**CNA port behaviour:** The player clicks directly on the desired square to place a mark
(no separate "Send Move" confirmation needed).  After game over, **N** starts a new game
and **ESC** exits.  No rendered button widgets.
**Root cause:** The original buttons rely on `SpriteFont`-rendered labels and touch
gesture events; the simpler direct-click model fits a desktop better and removes the
need for a separate confirmation step.
**Tracked in:** not planned (platform adaptation)

## Portrait phone layout adapted to landscape desktop window
**XNA behaviour:** Designed for a 480×800 portrait phone screen.  Board centred in
the middle third; status text and buttons occupy the upper and lower thirds.
**CNA port behaviour:** 800×600 landscape window; board centred horizontally and
slightly above centre; status text displayed below the board.
**Root cause:** Phone portrait layout does not translate directly to a desktop window.
**Tracked in:** not planned (layout adaptation)

## SpriteFont buttons omitted
**XNA behaviour:** Button labels ("Send Move", "New Game", "Exit") are drawn using
`SpriteFont` (`ButtonFont`).
**CNA port behaviour:** No button widgets rendered; keyboard shortcuts used instead
(see "Button widget" entry above).
**Root cause:** Follows from the input model change.
**Tracked in:** not planned

## Font: "Moire" substituted with a generated bitmap font
**XNA behaviour:** `TextFont.spritefont` and `ButtonFont.spritefont` both specify
`<FontName>Moire</FontName>` (a Windows-only font), at 40pt and 20pt respectively —
`TextFont` for the status text, `ButtonFont` for the three button labels.
**CNA port behaviour:** A single `Content/font.font.json` + `font.png` atlas is
generated via `tools/make_font.py` (substituting an available TrueType font), at a
much smaller pixel size suited only to the single-line status text at the bottom of
the board — the second, button-sized font was dropped along with the button widgets.
**Root cause:** "Moire" is a proprietary Windows font not freely distributable, and
CNA has no `.spritefont`/TTF-at-runtime pipeline; `make_font.py` bakes a static atlas
instead, per this repo's standard SpriteFont substitution convention.
**Tracked in:** not planned (acceptable substitute); DEFERRED.md item 2 (SpriteFont
pipeline) already resolved for the atlas mechanism itself.

## Fullscreen and 30 fps target omitted
**XNA behaviour:** The constructor sets `graphics.IsFullScreen = true` and
`TargetElapsedTime = TimeSpan.FromTicks(333333)` (~30 fps), commented "Frame rate is
30 fps by default for Windows Phone."
**CNA port behaviour:** `TicTacToeGame`'s constructor sets neither — the game runs
windowed at 800x600 and at CNA's `Game` default timestep (~60 fps), twice the
original's frame rate.
**Root cause:** Phone-specific settings outside the scope of a desktop port; several
other Windows-Phone-derived samples in this repo do port the explicit 30 fps
timestep, but this sample's port did not.
**Tracked in:** not planned.

## GamePad Back button dropped, keyboard-only exit
**XNA behaviour:** `Update` unconditionally polls
`GamePad.GetState(PlayerIndex.One).Buttons.Back` every frame (in addition to the
touch-driven "Exit" button) and exits the game when pressed.
**CNA port behaviour:** Only `Keyboard::GetState().IsKeyDown(Keys::Escape)` is
polled; no gamepad equivalent is wired up (CNA's `GamePad` API exists and is used
elsewhere in this repo, e.g. `GamePad::GetState`/`IsButtonDown`).
**Root cause:** Simplification for a desktop-only port; not a CNA limitation.
**Tracked in:** not planned

## First-turn probability changed from 40/60 to 50/50
**XNA behaviour:** `GameInitialize` (and `newGameButton_Click`'s restart) picks the
first player with `random.Next(0, 10) > 5` — true for 4 of the 10 possible values
(6,7,8,9), so the human player goes first only 40% of the time and the AI goes first
60% of the time.
**CNA port behaviour:** `InitBoard()`/`ResetGame()` use `random_.Next(0, 2) == 0` to
pick the AI, giving a fair 50/50 split between player-first and AI-first games.
**Root cause:** Port simplification/oversight — the original's skewed 40/60 odds
were not reproduced; CNA's `System::Random::Next(min, max)` has the same exclusive-
upper-bound semantics as .NET's, so the exact odds could be replicated with
`random_.Next(0, 10) > 5`.
**Tracked in:** not planned (cosmetic gameplay-balance difference, not a CNA
limitation).
