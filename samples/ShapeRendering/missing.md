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

## SpriteBatch: unused placeholder in XNA vs. F1-help-only use in the port

**XNA behaviour:** `new SpriteBatch(GraphicsDevice)` is created in
`LoadContent` (`ShapeRenderingSampleGame.cs:48`) but is never used for
drawing anywhere in the sample — no texture or text overlay is ever drawn
with it. It appears to be unused placeholder/leftover code.

**CNA port behaviour:** `ShapeRenderingGame.hpp` also creates a
`SpriteBatch` (`helpSpriteBatch_`, see `LoadContent()`), but unlike the
original this one IS used — exclusively to draw the CNA-only F1 help
overlay (`helpTexture_` / `Content/help.png`, drawn in `Draw()`). No
SpriteBatch is used for any purpose equivalent to the original (which has
none), so 3D shape-rendering behavior parity with the XNA sample is
unaffected.

**Root cause:** The F1 help overlay is a CNA-wide sample addition (see
CLAUDE.md "F1 Help Overlay") with no XNA equivalent; it happens to reuse
the same `SpriteBatch` type the original declared for its own, unrelated
and unused, purposes.

**Tracked in:** N/A — F1 overlay is a documented CNA addition, not a gap.

---

No known differences beyond the two above.
