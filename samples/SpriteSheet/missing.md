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

**Root cause:** CNA has no Content Pipeline.

**Tracked in:** DEFERRED.md item 6 (Model / pipeline loading).

---

## DrawString calls omitted

**XNA behaviour:** Two `spriteBatch.DrawString(spriteFont, ...)` calls render
text labels: "Here are some individual sprites, all stored in a single sprite
sheet:" (left panel) and "And here is the combined sprite sheet texture:"
(right panel).

**CNA port behaviour:** Both `DrawString` calls are omitted. The rest of the
drawing (spinning cat, glow animation, checker background, sprite sheet
texture) is unchanged.

**Root cause:** CNA does not yet support SpriteFont.

**Tracked in:** DEFERRED.md item (SpriteFont / DrawString).

---

## No known differences otherwise

The packing algorithm, sprite rotation, glow frame animation, LinearWrap
checkerboard background, and sprite sheet texture overlay all match the
XNA 4.0 original.
