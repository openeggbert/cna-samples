# Deferred Implementation Items

This file tracks features, API gaps, and asset-pipeline issues that block
sample ports from compiling or running correctly.  Items are roughly ordered
by how many samples they affect.

Each item records:
- **What is missing** — the specific gap in CNA or sharp-runtime
- **Where to implement** — which repository and rough file/module
- **Blocked samples** — which sample ports are waiting on this
- **Effort estimate** — rough size: S / M / L / XL

---

## 1. XNB Asset Pipeline

**What is missing:**
XNA 4.0 samples ship with compiled `.xnb` binary assets (textures, fonts, models,
audio).  CNA does not and will not support the XNB format.

**What needs to happen instead:**
All `.xnb` assets in each sample must be converted to open formats before a
CNA sample can load them:

| XNB asset type | Replacement format |
|---|---|
| Texture2D | PNG (or any format SDL_image supports) |
| SpriteFont | CNA SpriteFont from TTF / bitmap strip |
| Model | glTF 2.0 or OBJ |
| SoundEffect | OGG Vorbis or WAV |
| Song (music) | OGG Vorbis or MP3 |
| Effect (shader) | GLSL / HLSL depending on backend |

**Where to implement:**
- Asset extraction: use `MonoGame.Content.Builder` or `mgcb` CLI to convert
  existing XNB files to source assets.
- CNA ContentManager must then load each open format directly.

**Blocked samples:** ALL samples that use Content.Load<T>()

**Effort:** M per sample (asset conversion) + L for ContentManager loader additions

---

## 2. SpriteFont Loading

**What is missing:**
`Content.Load<SpriteFont>("hudfont")` used in Primitives3D and many other samples.
CNA's ContentManager has no SpriteFont loader; `SpriteFont` rendering support may
itself be partial.

**Where to implement:** `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`
and `cna/include/Microsoft/Xna/Framework/Graphics/SpriteFont.hpp`.

**Blocked samples:** Primitives3D, Platformer, and all HUD-drawing samples.

**Effort:** L

---

## 3. EasyGL: DiffuseColor ignored for VertexPositionColor meshes

**What is missing:**
`EasyGLGraphicsBackend::SelectProgram` selects the GLSL shader program purely by
vertex **stride** (byte size).  `VertexPositionColor` has stride 16 → falls into
`default: EnsureColored3DProgram()`.  That shader has only `FragColor = vColor`
(vertex color) and **no `uDiffuseColor` uniform** — so `loc_diffuse = -1` and
`BindDrawParams` never uploads `BasicEffect.DiffuseColor` to the GPU.

Consequence: `BasicEffect.DiffuseColor` (and the `setDiffuseColorProperty()` setter)
is silently ignored for any geometry stored in a `VertexPositionColor` vertex buffer.
In practice the mesh always renders in white.

**Where to implement:** `cna/src/CNA/Internal/Backends/EasyGL/EasyGLGraphicsBackend.cpp`,
`EnsureColored3DProgram()` — add `uDiffuseColor` uniform and multiply:
`FragColor = vColor * uDiffuseColor`.  When `params.vertexColorEnabled == false`,
the vertex color should be treated as `vec4(1.0)` so only `uDiffuseColor` tints the
output (needed for `BasicEffect` without `VertexColorEnabled`).

**Blocked samples:** Primitives3D (color change via B key produces no visible effect).

**Effort:** S

---

## 4. EasyGL: Wireframe rendering not possible on OpenGL ES

**What is missing:**
`FillMode::WireFrame` in `RasterizerState` is silently ignored by the EasyGL backend
(see `EasyGLGraphicsBackend.cpp` line with comment
`// FillMode::WireFrame not supported in OpenGL ES — silently ignored`).

Desktop OpenGL has `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`, but **OpenGL ES 3.x
does not expose this function at all** — it is a fundamental API omission, not a
driver bug.

**Where to implement (workaround in CNA):**
When `fillMode == WireFrame`, `DrawIndexedPrimitivesEx` could emit each triangle
as three `GL_LINES` instead of one `GL_TRIANGLES`, effectively emulating wireframe.
This requires rewriting index data on the fly (or maintaining a line-index buffer).
Alternatively target OpenGL ES 3.2+ which exposes `GL_OES_polygon_mode` on some
drivers, but that is not universally available.

**Blocked samples:** Primitives3D (Y key wireframe toggle has no visual effect).

**Effort:** M

---

## 5. VertexPositionNormal (3D lit rendering)

**What is missing:**
XNA's `VertexPositionNormal` struct and a corresponding lit shader in the backend.
The Primitives3D port uses `VertexPositionColor` as a workaround (flat shading only).

**Where to implement:**
- Add `VertexPositionNormal.hpp` to `cna/include/Microsoft/Xna/Framework/Graphics/`
- Add a normal-lit GLSL shader to the EasyGL backend
- Extend `VertexBuffer::SetData` to accept `VertexPositionNormal*`

**Blocked samples:** Primitives3D (lighting), SkinningSample, NormalMapping,
HeightmapCollision, BillboardSample, and all samples using `BasicEffect` with lighting.

**Effort:** L

---

## 6. Model Loading (ContentManager.Load<Model>)

**What is missing:**
`Content.Load<Model>("foo")` is not implemented.  Many samples rely on `.xnb`
model files (originally from FBX/X sources).

**Where to implement:**
- Add a glTF 2.0 model loader to CNA's ContentManager
- `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`
- Depends on a glTF parser library (e.g., `cgltf` or `tinygltf`)

**Blocked samples:** ChaseCamera, HeightmapCollision, SkinningSample, MarbleMaze,
ShipGame, RolePlayingGame, and many more.

**Effort:** XL

---

## 7. Audio (SoundEffect, SoundEffectInstance, Song)

**What is missing:**
CNA stubs exist for the audio namespace but actual playback is not implemented
for all platforms.  Samples that rely on `SoundEffect.Play()`, `MediaPlayer.Play()`,
or `AudioEmitter`/`AudioListener` 3D audio will either fail or produce no sound.

**Where to implement:** `cna/src/Microsoft/Xna/Framework/Audio/`

**Blocked samples:** Audio3DSample, SoundAndMusic, Platformer (music), and any
sample with background music or sound effects.

**Effort:** L

---

## 8. SpriteBatch.DrawString (in-game text rendering)

**What is missing:**
`SpriteBatch.DrawString(spriteFont, text, position, color)` requires a working
`SpriteFont` (see item 2).  Without it, HUD text in all 3D samples is absent.

**Where to implement:** Depends on SpriteFont (item 2).

**Blocked samples:** Primitives3D (controls overlay), Platformer (score HUD),
and virtually all samples with on-screen text.

**Effort:** M (after SpriteFont is done)

---

## 9. Mouse Input — Viewport.AspectRatio

**What is missing (minor):**
`Viewport` has no `AspectRatio` convenience property.  Currently computed manually
as `(float)Width / (float)Height` in each sample.

**Where to implement:** `cna/include/Microsoft/Xna/Framework/Graphics/Viewport.hpp`

**Blocked samples:** Non-blocking workaround exists; cosmetic only.

**Effort:** S

---

## 10. GamePad.IsButtonDown shortcut

**What is missing (minor):**
In XNA, `GamePad.GetState(PlayerIndex.One).Buttons.Back` is a direct member access.
In CNA, `Buttons` is accessed via `getButtonsProperty()` and `Back` via `getBackProperty()`.
`GamePadState.IsButtonDown(Buttons::Back)` works correctly and is used in the ports.

**Where to implement:** Not strictly needed; `IsButtonDown` is the cleaner API anyway.

**Effort:** —

---

## Summary Table

| # | Feature | Repo | Effort | Samples blocked |
|---|---|---|---|---|
| 1 | XNB → open format pipeline | all | M/sample | all |
| 2 | SpriteFont loading | cna | L | many |
| 3 | EasyGL: DiffuseColor ignored for VertexPositionColor | cna | S | Primitives3D |
| 4 | EasyGL: Wireframe mode (OpenGL ES has no glPolygonMode) | cna | M | Primitives3D |
| 5 | VertexPositionNormal + lit shader | cna | L | many |
| 6 | Model loading (glTF) | cna | XL | many |
| 7 | Audio playback | cna | L | many |
| 8 | SpriteBatch.DrawString / SpriteFont | cna | M | most |
| 9 | Viewport.AspectRatio | cna | S | 0 (workaround) |
| 10 | GamePadButtons direct access | cna | — | 0 (workaround) |
