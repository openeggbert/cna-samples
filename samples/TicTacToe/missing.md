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
