# Missing / Differences from XNA 4.0 original

## Player index displayed as number, not enum name

**XNA behaviour:** `"Player " + inputManager.PlayerIndex + " input  "` concatenates
the `PlayerIndex` enum and produces `"Player One input  "` / `"Player Two input  "`.

**CNA port behaviour:** Displays `"Player 1 input  "` / `"Player 2 input  "` using
`std::to_string(i + 1)`. Functionally equivalent, style differs.

**Root cause:** C++ enums don't stringify via `+`; `std::to_string` is idiomatic.

**Tracked in:** not planned (cosmetic difference).

---

## No known differences otherwise

Direction detection, input buffer management, move matching (timeout, merge
window, sub-move, sequence matching), move-name labels, player input rows,
and the drop-shadow DrawString all match the XNA 4.0 original.
