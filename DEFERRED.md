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

## 6. Model Asset Conversion (FBX/X → .model.json) ✅ CNA SUPPORTS STATIC MODELS

**What is missing:**
`Content.Load<Model>()` IS implemented in CNA via `.model.json` descriptor +
binary vertex/index files, and `Model`/`ModelMesh`/`ModelBone`/`ModelMeshPart` all
work end-to-end for a **static, rigid bone hierarchy** — proven by
`examples/easygl_model_draw_test.cpp` (2-bone model, `CopyAbsoluteBoneTransformsTo`,
full `Model::Draw` chain). For any sample whose model is static (or only needs rigid
parent-child bone transforms, not skeletal animation playback), the gap is purely
**asset conversion**: XNA samples ship `.x` and `.fbx` model files that must be
converted to CNA's `.model.json` format.

**This item covers static models only.** If a sample needs to *play* a skeletal
animation (walk cycles, skinned character rigs, etc. — not just render a rigid
multi-part mesh), see item 13 below instead — that capability does not exist in CNA
yet, regardless of asset conversion.

**Caveat found while investigating SplitScreen (2026-07-04): the "asset conversion
only, no CNA code changes needed" claim above only holds for models with a *single*
bone (the common case so far — Ground, tank-as-rendered-by-CameraShake, etc., which
never move any part independently).** `examples/easygl_model_draw_test.cpp`'s 2-bone
proof constructs `ModelBone`s directly in C++ — it does **not** exercise the
`.model.json` *file format*/reader's ability to build a multi-bone hierarchy from
JSON. In reality, `ContentManager.cpp`'s `ModelTypeReader::Read()` only ever creates
one synthetic "Root" bone total; every mesh parsed from `.model.json`'s `"meshes"`
array is left with a null parent bone (`ModelMesh` doesn't even have a setter for it).
So any sample needing independently-animated rigid parts (multiple named bones, one
per moving mesh, XNA's common non-skinned "rig" pattern — e.g. a tank with rotating
wheels/turret) is blocked on a small CNA *code* change, not just conversion — see
`samples/SplitScreen/missing.md` for the full write-up (exact files/functions to
change, and why `samples/CameraShake/Content/tank.model.json` would need zero
regeneration once the reader supports it, since its mesh names already match the
bone names a consumer like `Tank.cs` expects).

**What needs to happen:**
- For single-bone (fully static) models: convert source `.x`/`.fbx` files to
  `.model.json` + binary buffers. Tools: `assimp export` + `tools/obj2model.py`, or
  `tools/fbx_ascii2model.py`. One conversion per model file per sample. No CNA code
  changes needed — proven twice (CameraShake, PerformanceMeasuring).
- For multi-bone rigid-part models (SplitScreen and similar — see above): also needs
  a `ModelMesh` parent-bone setter plus a `ModelTypeReader::Read()` change to build one
  real bone per mesh (or parse an explicit bone hierarchy from the JSON) — a CNA code
  change, not just conversion.

**Blocked samples (static geometry only, conversion-only gap):** CameraShake,
BloomSample (tank), Spacewar (Evolved ships/asteroids), ChaseCamera,
HeightmapCollision, MarbleMaze, ShipGame, and similar.

**Blocked samples (rigid multi-part bone hierarchy — needs the reader change above,
not just conversion):** SplitScreen (see its `missing.md`), SimpleAnimation,
TankOnAHeightMap, CustomModelClassSample, ModelViewerDemo (all animate the same
`tank.fbx`'s wheels/turret/cannon/hatch independently). (SkinningSample,
RolePlayingGame, and other *skinned*-animation samples are additionally blocked on
item 13 regardless.)

**Effort:** M per model (conversion) for single-bone models, no CNA code changes
needed. S–M CNA code change (once) to unblock the multi-bone rigid-part case, then M
per model as above.

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

## 13. Skeletal Animation Playback (AnimationClip / Keyframe / AnimationPlayer)

**What is missing:**
Unlike items 6 and 11 (which are pure asset-conversion gaps — CNA's own runtime
already works), this is a **real, unimplemented CNA capability**. `Model`/`ModelBone`
support a static or rigid bone *hierarchy* (proven working, see item 6), but there is
no equivalent of the standard XNA "Skinned Model Sample" pipeline types:
`AnimationClip`, `Keyframe`, `AnimationPlayer` (interpolates/advances a clip's
keyframes and produces a bone-transform array per frame) do not exist anywhere in
`cna/include` or `cna/src` (confirmed by grep — zero matches). There is also no
skeletal/skinning data in the `.model.json` format itself (no bone-weight-per-vertex,
no keyframe/clip section) — `ModelTypeReader` in `ContentManager.cpp` only parses a
static `"bones"` hierarchy, not animation data.

Note this is a different problem from `SkinnedEffect`'s GLSL shader
(`src/CNA/Internal/Backends/Vulkan/shaders/skinned3d.{vert,frag}.glsl`), which already
exists and can render a skinned mesh *given* a bone-transform array — the missing
piece is producing that per-frame bone-transform array from animation data in the
first place, plus the `.model.json` schema extension to carry vertex bone
weights/indices and keyframe data.

**What needs to happen:**
- Extend the `.model.json` schema (and `ModelTypeReader`) to carry per-vertex bone
  weights/indices and a keyframe/clip section (or a sibling `.animation.json`).
- Add `AnimationClip`/`Keyframe`/`AnimationPlayer` classes (or CNA-native
  equivalents) under `cna/include/Microsoft/Xna/Framework/` mirroring the XNA
  Skinned Model Sample's own helper types, since ported samples that use skeletal
  animation generally include copies of those exact helper classes and call into
  them the same way.
- Wire `AnimationPlayer`'s output bone-transform array into `Model::Draw`'s existing
  `boneTransform` parameter (already proven working for the static case in
  `examples/easygl_model_draw_test.cpp`) — this final wiring step may be small once
  the data model and player exist.
- A conversion tool step is also needed (FBX/X skeletal animation → the extended
  `.model.json`/`.animation.json`), same class of tooling work as item 6, but cannot
  be usefully built before the runtime side above exists to consume its output.

**Blocked samples:** SkinningSample, RolePlayingGame, and any other Phase 4 sample
whose model needs to *play* an animation rather than just render a static/rigid mesh
(most of Phase 4's remaining 9 samples — check each one individually, since some may
only need static geometry and are actually blocked on item 6 alone).

**Effort:** L/XL — real engine-level feature work in `cna`, not just content
conversion. Recommend prototyping the data model + player against one sample
(SkinningSample is the XNA reference sample this pipeline is named after) before
attempting the rest.

---

## Summary Table

| # | Feature | Repo | Effort | Samples blocked |
|---|---|---|---|---|
| 1 | XNB → open format pipeline | all | M/sample | all |
| 2 | SpriteFont loading | cna | L | many | ✅ done |
| 3 | EasyGL: DiffuseColor ignored for VertexPositionColor | cna | S | Primitives3D | ✅ done |
| 4 | EasyGL: Wireframe mode (OpenGL ES has no glPolygonMode) | cna | M | Primitives3D | ✅ done |
| 5 | VertexPositionNormal + lit shader | cna | L | many |
| 6 | Model asset conversion, static geometry (.x/.fbx → .model.json) | tools | M/model | many | CNA itself works |
| 7 | Audio playback | cna | — | — | ✅ done (SDL3_mixer) |
| 8 | SpriteBatch.DrawString / SpriteFont | cna | M | most | ✅ done |
| 9 | Viewport.AspectRatio | cna | S | 0 (workaround) |
| 10 | GamePadButtons direct access | cna | — | 0 (workaround) |
| 11 | Shader conversion (HLSL .fx → GLSL .shader.json) | tools | M/shader | many Phase 3+ | CNA itself works |
| 12 | RenderTarget2D | cna | — | — | ✅ done |
| 13 | Skeletal animation playback (AnimationClip/Keyframe/AnimationPlayer) | cna | L/XL | SkinningSample, RolePlayingGame, other animated Phase 4 samples | not started |
