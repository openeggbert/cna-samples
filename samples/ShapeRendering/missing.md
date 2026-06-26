# Missing / Differences from XNA 4.0 original

## [Conditional("DEBUG")] attributes removed

**XNA behaviour:** All `DebugShapeRenderer` methods are decorated with
`[Conditional("DEBUG")]` — in Release builds they compile to no-ops,
making debug rendering a zero-cost abstraction in shipped games.

**CNA port behaviour:** The `[Conditional]` attribute has no direct C++
equivalent.  All methods are always compiled and always execute.  For
production use this would need `#ifdef NDEBUG` guards, but for a sample
this makes no practical difference.

**Root cause:** C# conditional compilation attribute vs. C++ preprocessor.

**Tracked in:** Not planned (sample-only concern).

---

## SpriteBatch created but never used — omitted in port

**XNA behaviour:** `new SpriteBatch(GraphicsDevice)` is called in
`LoadContent` but the SpriteBatch is never used for drawing (no texture
or text overlay).

**CNA port behaviour:** SpriteBatch creation is omitted — CNA's SpriteBatch
requires a `Texture2D` argument and is not yet needed here.

**Root cause:** The original sample likely had placeholder code for future
HUD text that was never implemented.

**Tracked in:** N/A.

---

No known differences beyond the two above.
