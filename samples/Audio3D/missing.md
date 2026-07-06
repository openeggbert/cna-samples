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

## DopplerScale has no audible effect
**XNA behaviour:** `SoundEffect.DopplerScale = 0.1f` combined with `AudioEmitter.Velocity`/`AudioListener.Velocity` causes XAudio2/X3DAudio to apply a real-time pitch shift as the cat orbits past the listener (faster relative velocity = more pitch shift).
**CNA port behaviour:** `SoundEffect::setDopplerScaleProperty(0.1f)` is stored and `Apply3D` receives the same emitter/listener velocities, but no pitch shift is actually computed — Doppler is a documented no-op.
**Root cause:** CNA's audio backend is SDL3_mixer, which has no per-source velocity-based pitch primitive to hook into (no `F3DAudio` equivalent). `DopplerScale`/emitter velocity are API-complete but unconsumed.
**Tracked in:** CNA issue (`docs/xna-4-api-coverage.md` "Intentionally unsupported" — Doppler/velocity-based pitch shift, stored, never applied)

## 3D audio is pan + linear distance attenuation only
**XNA behaviour:** `SoundEffectInstance.Apply3D` computes a full X3DAudio spatialization (pan, distance attenuation, and elevation/HRTF cues) from the emitter/listener orientation vectors, so the cat sounds subtly different as it circles above/behind vs. directly in front of the listener.
**CNA port behaviour:** `Apply3D` reduces to stereo pan plus linear distance attenuation from `SoundEffect::DistanceScale`; there is no elevation or HRTF component, so orientation changes that aren't purely left/right produce no audible difference.
**Root cause:** SDL3_mixer has no positional-audio DSP graph; CNA approximates 3D sound at the engine layer (pan + distance) rather than the backend computing it, per `docs/xna-4-api-coverage.md` "Approximate" bucket.
**Tracked in:** CNA issue (SDL3_mixer 3D audio approximation, `P9-DOCS-006`)

## No other known functional differences
The cat/dog entity movement, camera fly-through controls, billboard rendering with
alpha-test masking, and checkered ground plane are all faithfully ported. GameComponent
(AudioManager) lifecycle is handled by `Game.Components` as in XNA.
