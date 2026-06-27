# Missing / Differences from XNA 4.0 original

## Textures converted from TGA to PNG
**XNA behaviour:** Loads `CatTexture.tga`, `DogTexture.tga`, `checker.tga` via ContentManager.
**CNA port behaviour:** Textures converted to PNG format (`CatTexture.png`, `DogTexture.png`, `checker.png`).
**Root cause:** CNA ContentManager resolves PNG by default; TGA source preserved in XNA archive.
**Tracked in:** not planned (functionally identical)

## SoundEffect.DistanceScale / DopplerScale are static properties
**XNA behaviour:** `SoundEffect.DistanceScale = 2000` and `SoundEffect.DopplerScale = 0.1` set class-level defaults.
**CNA port behaviour:** Uses `SoundEffect::setDistanceScaleProperty(2000.0f)` and `SoundEffect::setDopplerScaleProperty(0.1f)` — same semantics, different access pattern.
**Root cause:** CNA uses property accessor style for all XNA API members.
**Tracked in:** not planned

## No known functional differences
The 3D positional audio effect, cat/dog entity movement, camera fly-through controls,
billboard rendering with alpha-test masking, and checkered ground plane are all faithfully
ported. GameComponent (AudioManager) lifecycle is handled by `Game.Components` as in XNA.
