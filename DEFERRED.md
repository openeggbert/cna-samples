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

## 2. SpriteFont Loading ✅ RESOLVED

**What was missing:**
`Content.Load<SpriteFont>("hudfont")` — CNA now fully supports this via
`.font.json` + atlas PNG format (`SpriteFontTypeReader` in `ContentManager.cpp`).

**How to generate a font asset:**
```
python3 tools/make_font.py <path/to/font.ttf> <size_px> <Content/FontName>
```
This produces `Content/FontName.font.json` + `Content/FontName.png`.

**Already used in:** SafeArea (corner labels, A-toggle hint), InputSequence (move
names, player labels, drop-shadow DrawString).

---

## 3. EasyGL: DiffuseColor ignored for VertexPositionColor meshes ✅ RESOLVED

**Was:** `EasyGLGraphicsBackend::SelectProgram` selects the GLSL shader program purely
by vertex **stride**.  `VertexPositionColor` (stride 16) → `EnsureColored3DProgram()`,
whose shader was `FragColor = vColor` with **no `uDiffuseColor` uniform**, so
`loc_diffuse = -1` and `BindDrawParams` never uploaded `BasicEffect.DiffuseColor`.
The mesh always rendered white.

**Fix:** `EnsureColored3DProgram()` now declares `uniform vec4 uDiffuseColor` and the
fragment shader outputs `FragColor = vColor * uDiffuseColor`; `prog_colored_.loc_diffuse`
is wired to it.  `GpuDrawParams.diffuseColor` defaults to `{1,1,1,1}`, so BasicEffect Ex
draws that set no diffuse are unaffected.  The non-Ex user-primitive paths
(`DrawColoredPrimitives` / `DrawIndexedColoredPrimitives`) explicitly upload white,
since they carry no diffuse and the uniform would otherwise default to 0 (black).
(`cna/src/CNA/Internal/Backends/EasyGL/EasyGLGraphicsBackend.cpp`.)

**Verified:** Primitives3D B-key cycles the tint; the cube renders red (was white).

**Effort:** S

---

## 4. EasyGL: Wireframe rendering not possible on OpenGL ES ✅ RESOLVED

**Was:** `FillMode::WireFrame` in `RasterizerState` was silently ignored — OpenGL ES 3.x
has no `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`.

**Fix:** `ApplyRasterizerState` records a `wireframe_` flag when `FillMode::WireFrame`
is set.  The 3D draw paths then call the new `DrawWireframe` helper, which re-expands
each triangle into three `GL_LINES` edges (`(a,b),(b,c),(c,a)`) through a scratch
32-bit line index buffer and draws `GL_LINES` instead of `GL_TRIANGLES`.  Covers both
indexed and non-indexed triangle list / strip draws (honouring `startIndex` /
`baseVertex` / `vertexStart`).  (`cna/.../EasyGL/EasyGLGraphicsBackend.cpp`.)

**Verified:** Primitives3D Y-key shows the cube as an edge-only wireframe.

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

## 6. Model Asset Conversion (FBX/X → .model.json) ✅ CNA SUPPORTS MODELS

**What is missing:**
`Content.Load<Model>()` IS implemented in CNA via `.model.json` descriptor +
binary vertex/index files.  The gap is **asset conversion**: XNA samples ship
`.x` and `.fbx` model files that must be converted to CNA's `.model.json` format.

**What needs to happen:**
- Convert source `.x`/`.fbx` files to `.model.json` + binary buffers
- Tools: Blender export script, `assimp` CLI, or custom converter
- One conversion per model file per sample

**Blocked samples:** CameraShake, BloomSample (tank), Spacewar (Evolved ships/asteroids),
ChaseCamera, HeightmapCollision, SkinningSample, MarbleMaze, ShipGame, RolePlayingGame,
and many more.

**Effort:** M per model (conversion) — no CNA code changes needed

---

## 7. Audio (SoundEffect, SoundEffectInstance, Song) ✅ RESOLVED

**What was missing:**
Audio was thought to be unimplemented but CNA fully supports audio via SDL3_mixer
(`SOUND_ENABLED`). `SoundEffect`, `SoundEffectInstance`, `Song`, `MediaPlayer`,
`SoundBank`, `WaveBank`, `AudioEngine`, and `Cue` are all implemented.

**Status:** Fully working. No blocker for audio samples.

---

## 8. SpriteBatch.DrawString (in-game text rendering) ✅ RESOLVED

**What was missing:**
`SpriteBatch.DrawString(spriteFont, text, position, color)`.

**Status:** Fully implemented in CNA. Depends on SpriteFont (item 2 above), which
is also resolved. Use `tools/make_font.py` to generate font assets, then load with
`Content.Load<SpriteFont>("FontName")`.

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

## 11. Custom User Effect Shader Conversion (HLSL .fx → .shader.json) ✅ CNA SUPPORTS CUSTOM EFFECTS

**What is missing:**
`Content.Load<Effect>()` IS implemented in CNA via `.shader.json` descriptor
referencing GLSL vertex + fragment shader files.  The gap is **shader conversion**:
XNA samples use HLSL `.fx` files that must be rewritten as GLSL and described
via `.shader.json`.

**What needs to happen:**
- Translate HLSL `.fx` shader logic to GLSL (vertex + fragment)
- Create `.shader.json` descriptor per effect
- Tools: manual port or `SPIRV-Cross` + `dxc` pipeline

**Blocked samples:** BloomSample (3 shaders), DistortionSample, NonPhotoRealistic,
NormalMapping, PerPixelLighting, VertexLighting, RimLighting, ShadowMapping,
ShatterEffect, Particles3D, and all Phase 3+ shader samples.

**Effort:** XL

---

## 12. RenderTarget2D ✅ RESOLVED

**What was missing:**
`RenderTarget2D` and `GraphicsDevice.SetRenderTarget()` were thought to be missing
but are fully implemented in CNA (`RenderTarget2D.hpp` / `.cpp`, backend framebuffer
support in both EasyGL and Vulkan backends).

**Status:** Fully working. No blocker.

---

## Summary Table

| # | Feature | Repo | Effort | Samples blocked |
|---|---|---|---|---|
| 1 | XNB → open format pipeline | all | M/sample | all |
| 2 | SpriteFont loading | cna | L | many | ✅ done |
| 3 | EasyGL: DiffuseColor ignored for VertexPositionColor | cna | S | Primitives3D | ✅ done |
| 4 | EasyGL: Wireframe mode (OpenGL ES has no glPolygonMode) | cna | M | Primitives3D | ✅ done |
| 5 | VertexPositionNormal + lit shader | cna | L | many |
| 6 | Model asset conversion (.x/.fbx → .model.json) | tools | M/model | many | CNA itself works |
| 7 | Audio playback | cna | — | — | ✅ done (SDL3_mixer) |
| 8 | SpriteBatch.DrawString / SpriteFont | cna | M | most | ✅ done |
| 9 | Viewport.AspectRatio | cna | S | 0 (workaround) |
| 10 | GamePadButtons direct access | cna | — | 0 (workaround) |
| 11 | Shader conversion (HLSL .fx → GLSL .shader.json) | tools | M/shader | many Phase 3+ | CNA itself works |
| 12 | RenderTarget2D | cna | — | — | ✅ done |
