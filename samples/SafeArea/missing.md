# Missing / Differences from XNA 4.0 original

## DrawString corner labels omitted

**XNA behaviour:** `DrawOverlays()` draws "Top Left", "Top Right", "Bottom Left",
"Bottom Right" text labels in the four corners of the title-safe area using
`AlignedSpriteBatch.DrawString(font, ...)`.

**CNA port behaviour:** `DrawOverlays()` is omitted entirely. The rest of the
drawing (cat, scrolling background) is unchanged.

**Root cause:** CNA has no SpriteFont support yet.

**Tracked in:** DEFERRED.md (SpriteFont / DrawString).

---

## SafeAreaOverlay not added to Components

**XNA behaviour:** On Xbox Debug builds, `SafeAreaOverlay` is created and added
to `Components`; it superimposes a translucent red border around the title-safe
area to aid visual verification.

**CNA port behaviour:** `SafeAreaOverlay.hpp` is included in the port but never
instantiated. On PC (`#if XBOX && DEBUG` is never true), the original also skips
it, so the visible output is identical.

**Root cause:** CNA targets desktop Linux; the title-safe overlay is Xbox-only
in the original.

**Tracked in:** not planned (matches the original desktop behaviour).

---

## No known differences otherwise

Cat movement, scrolling background, camera clamping, and `Viewport.TitleSafeArea`
usage all match the XNA 4.0 original.
