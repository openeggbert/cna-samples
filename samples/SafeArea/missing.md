# Missing / Differences from XNA 4.0 original

## SafeAreaOverlay always shown (not Xbox-Debug-only)

**XNA behaviour:** `SafeAreaOverlay` is only created on Xbox Debug builds
(`#if XBOX && DEBUG`); on PC it is null and therefore never shown.

**CNA port behaviour:** `SafeAreaOverlay` is always created and added to
`Components`, so the translucent red safe-area border is always visible
(and can be toggled with A / Keyboard-A). The toggle prompt text is also
always shown.

**Root cause:** CNA targets desktop Linux; the Xbox conditional was simplified
to always-on since the safe-area overlay aids visual verification on any platform.

**Tracked in:** not planned (intentional simplification).

---

## No known differences otherwise

Cat movement, scrolling background, camera clamping, `Viewport.TitleSafeArea`
usage, `AlignedSpriteBatch` corner labels, and the A-toggle all match the
XNA 4.0 original.
