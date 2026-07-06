# Missing / Differences from XNA 4.0 original

## Font: DejaVu Sans (Regular) instead of Miramonte (Bold)

**XNA behaviour:** `Content/Font.spritefont` specifies `<FontName>Miramonte</FontName>`,
`<Size>14</Size>` (points), `<Style>Bold</Style>`.

**CNA port behaviour:** `Content/Font.font.json` + `Font.png` were generated with
`tools/make_font.py /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf 16 …` — DejaVu Sans
**Regular** at 16px (confirmed byte-for-byte by regenerating the atlas with these exact
arguments and diffing the glyph table). Miramonte is not freely distributable; DejaVu Sans
is the standard substitute used across CNA samples, but the weight (Regular, not Bold)
does not match the original's `Bold` style.

**Root cause:** Miramonte is a licensed Windows font, unavailable on Linux. DejaVu Sans is
the project's standard substitute (see CLAUDE.md Assets section), but no Bold variant was
selected when this atlas was generated.

**Tracked in:** not planned (acceptable substitute; weight mismatch is cosmetic).

---

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
