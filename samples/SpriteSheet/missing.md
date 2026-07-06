# Missing / Differences from XNA 4.0 original

## Sprite packing done at runtime instead of Content Pipeline

**XNA behaviour:** `SpriteSheetProcessor.cs` runs `SpritePacker.cs` at build
time, reading `SpriteSheet.xml` (which lists `cat.tga`, `glow1-7.png`),
packing them into a single atlas texture, and serialising the result as a
compiled `.xnb` file. The game loads the pre-packed atlas with
`Content.Load<SpriteSheet>("SpriteSheet")`.

**CNA port behaviour:** The same packing algorithm (GuessOutputWidth,
PositionSprite, FindIntersecting, 1 px padding) runs at LoadContent() time
in `SpriteSheet::Build()`. Individual sprite images are loaded as
`Texture2D`, their pixel data is read via `GetData`, packed into a
`std::vector<Color>` atlas, and uploaded to the GPU via `Texture2D::SetData`.
The runtime result is identical to the XNA build-time atlas.

**Root cause:** CNA has no build-time, pluggable `ContentProcessor`-chaining
extensibility point (custom processors like `SpriteSheetProcessor.cs` /
`SpritePacker.cs` can't run as part of an offline content build).

**Tracked in:** DEFERRED.md item 18 (Content-pipeline processor
extensibility). Item 6 is unrelated (that one covers Model/FBX asset
conversion only).

---

## DrawString calls omitted (SpriteFont never loaded)

**XNA behaviour:** Two `spriteBatch.DrawString(spriteFont, ...)` calls render
text labels: "Here are some individual sprites, all stored in a single sprite
sheet:" (`SpriteSheetGame.cs:125-127`) and "And here is the combined sprite
sheet texture:" (`SpriteSheetGame.cs:203-205`), using a `hudFont` SpriteFont
loaded via `Content.Load<SpriteFont>("hudFont")`.

**CNA port behaviour:** Both `DrawString` calls are omitted, and no
`hudFont` SpriteFont asset is loaded anywhere in `SpriteSheetGame.hpp`. The
rest of the drawing (spinning cat, glow animation, checker background,
sprite sheet texture) is unchanged.

**Root cause:** NOT a current CNA limitation — SpriteFont loading and
`SpriteBatch.DrawString` are both fully implemented in CNA (DEFERRED.md
items 2 and 8, both marked ✅ resolved) and are already used successfully by
other ported samples (SafeArea, InputSequence). This port simply never
generated a `hudFont` atlas via `tools/make_font.py` or added the two
`DrawString` calls — an open port omission, not a framework gap.

**Tracked in:** Not a DEFERRED.md item (the underlying feature already
exists) — open port gap; a future session could add
`Content/hudFont.font.json`/`.png` via `tools/make_font.py` and restore the
two `DrawString` calls.

---

## No known differences otherwise

The packing algorithm, sprite rotation, glow frame animation, LinearWrap
checkerboard background, and sprite sheet texture overlay all match the
XNA 4.0 original.
