# Missing / Differences from XNA 4.0 original

## Move names and player labels not drawn

**XNA behaviour:** Move names (e.g. "Jump", "Fireball") are drawn above their
button-icon sequence using `spriteFont.DrawString`. Player input rows are
prefixed with "Player 0 input" and the currently active move name is shown
in red.

**CNA port behaviour:** All `DrawString` calls are omitted. Only the button-icon
sequences (directional arrows, gamepad face buttons) are drawn. The combo
detection logic and input buffer are fully functional.

**Root cause:** CNA has no SpriteFont support yet.

**Tracked in:** DEFERRED.md (SpriteFont / DrawString).

---

## MeasureMove ignores text height

**XNA behaviour:** `MeasureMove` includes the text label height when computing
layout row height, so each move row is tall enough for both text and icons.

**CNA port behaviour:** `MeasureMove` returns only the icon sequence size
(height = `padFaceTexture.Height`). Layout is otherwise identical.

**Root cause:** consequence of omitting SpriteFont.

**Tracked in:** DEFERRED.md (SpriteFont / DrawString).

---

## No known differences otherwise

Direction detection, input buffer management, move matching (timeout, merge
window, sub-move, sequence matching) all match the XNA 4.0 original.
