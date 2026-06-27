# Missing / Differences from XNA 4.0 original

## SpriteFont / state text omitted
**XNA behaviour:** Draws "Tank State: Chasing/Wander/Caught" and "Mouse State: Evading/Wander" with a drop-shadow using DrawString.
**CNA port behaviour:** No text is drawn.
**Root cause:** CNA has no SpriteFont support yet.
**Tracked in:** DEFERRED.md
