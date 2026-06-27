# Missing / Differences from XNA 4.0 original

## Color names replaced with RGBA literals
**XNA behaviour:** Uses `Color.Red` and `Color.CornflowerBlue` named constants.
**CNA port behaviour:** Equivalent RGBA values used directly: `Color(255,0,0,255)` and `Color(100,149,237,255)`.
**Root cause:** CNA Color does not expose named static constants yet.
**Tracked in:** CNA issue (minor convenience gap).
