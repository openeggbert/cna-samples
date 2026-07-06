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

## 5. VertexPositionNormal (3D lit rendering) — ✅ RESOLVED for Model-based samples (2026-07-06)

**Corrected 2026-07-06:** this item claimed CNA had no lit-shader path for
per-vertex-normal 3D rendering at all. A live check found that's no longer true:
`VertexPositionNormalTexture` (position+normal+texcoord — the vertex format
`ContentManager.cpp`'s `ModelTypeReader` already produces for any `.model.json`
with normal data, confirmed by reading the reader's `SetData` dispatch) has a real,
**tested and passing** directional-lighting path in the EasyGL backend: built and
ran `cna_test_easygl_basiceffect_combinations` live — case "(e) Directional
lighting — VertexPositionNormalTexture, white tex, red light → reddish pixel"
passes (exit code 0). The Vulkan backend also has a dedicated
`lit_textured3d.{vert,frag}.glsl` shader pair. This unblocks all 9 samples
previously listed here, since every one of them renders via `Content.Load<Model>`
(confirmed by grep of each one's own C# original) → `VertexPositionNormalTexture` →
this now-working lit path, not a bare, texture-less vertex struct:
**LensFlareSample** (#041), **Graphics3DSample** (#046, `Spaceship.cs` loads
`content.Load<Model>("Models/spaceship")` — note lowercase `content` local var
masked this from a naive `Content.Load` grep), **PickingSample** (#047 — its own
dead-code `GeometricPrimitive.cs`/bare `VertexPositionNormal` struct is *not*
compiled into the `.csproj`; the real runtime path is `Content.Load<Model>`),
**TrianglePickingSample** (#048), **HeightmapCollisionSample** (#049),
**CustomModelClassSample** (#052, loads via `Content.Load<CustomModel>` — a
sample-defined custom content type; port can just use the standard
`tools/obj2model.py` conversion + stock `Model` like every other sample, since
CNA has no generic custom-`ContentTypeReader` extensibility anyway — see item 18),
**InverseKinematics** (#057), **ChaseCamera** (#058), and **MarbleMaze**'s (#061)
EX2/End build. Update each sample's `missing.md`/`PLAN.md` status and re-attempt
porting.

**Still open — Primitives3D specifically:** Primitives3D's original C# ships its
**own**, sample-authored, texture-**less** `VertexPositionNormal` struct
(`Primitives3D/VertexPositionNormal.cs`, its own `IVertexType` — not a built-in XNA
type) for its procedurally-generated primitives (sphere/cylinder/etc. have no UV
data). CNA has no texture-less normal-lit vertex format, so this specific sample
still can't get real per-vertex lighting without either (a) a small CNA addition
(a `VertexPositionNormal` variant + wiring it into the same lit shader), or (b) a
port-side workaround (assign a dummy/unused texcoord to every vertex and use the
already-working `VertexPositionNormalTexture` instead) — the port-side workaround
is likely the pragmatic choice given `VertexPositionNormalTexture` is proven and no
second sample needs a texture-less variant.

**Effort:** — (done in `cna` for the Model-based case) / S (Primitives3D-specific
workaround, port-side, no CNA change needed)

---

## 14. TextureCube content loading (`Content.Load<TextureCube>`)

**What is missing:**
`ContentManager.cpp` has no `TextureCubeTypeReader` registered — `Content.Load<TextureCube>(...)`
throws, even though the underlying pieces it needs already exist: CNA's `TextureCube`
class and DDS decoding are both implemented and proven working (see the
`easygl_texturecube_*` example/tests), and `EnvironmentMapEffect` is also already
implemented.

**Where to implement:** Add a `TextureCubeTypeReader : ContentTypeReader<Graphics::TextureCube>`
to `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`'s built-in type readers
(same shape as the existing `Texture2DTypeReader`), reading a `.dds` cubemap file.

**Blocked samples:** RimLighting (#037) — confirmed via direct source audit
(2026-07-05) to have **zero** custom `.fx` files; it uses stock `EnvironmentMapEffect`
+ a `TextureCube` (`OutputCube.dds`) purely via `Content.Load<TextureCube>`. This is
the only real gap — unlike the rest of Phase 3, RimLighting does not need the XL
HLSL→GLSL shader pipeline (item 11) at all.

**Effort:** S

---

## 15. Accelerometer/sensor platform reality — NOT a hard blocker where a fallback fits

**What is actually true (confirmed by reading `cna/src/Microsoft/Devices/Sensors/Accelerometer.cpp`
directly, 2026-07-05):** `Microsoft::Devices::Sensors::Accelerometer` is a real,
working, SDL3 `SDL_Sensor`-backed implementation — **not** a stub. `getIsSupportedProperty()`
gates on `CNA::getCurrentPlatform()` being `Android`, `iOS`, **or** `Desktop` (not
Android-only), then does a real probe (`SDL_InitSubSystem`/`SDL_GetSensors`/
`SDL_OpenSensor`) for actual sensor hardware. On this project's development machine
(desktop Linux, no physical accelerometer chip), that probe correctly returns `false`
— that is a hardware-absence result, not a platform restriction, and the same code
path would return `true` on a real Android/iOS device or a desktop/laptop with a
physical accelerometer (e.g. a 2-in-1 tablet). `Gyroscope` has the identical
Android/iOS/Desktop-gated, real-probe shape (see `Gyroscope.cpp`).

**Established working pattern (proven three times — Yacht, SnowShovel, Bounce):**
when `getIsSupportedProperty()` is false (this desktop, the normal case), the sample
falls back to keyboard/gamepad/touch input that the *original* XNA sample's own
Windows desktop build already shipped (Yacht/SnowShovel/Bounce are all Windows Phone
ports that had a `#if WINDOWS_PHONE` accelerometer branch **and** a working non-phone
input branch in the same shipped C#) — no invented control scheme, just wiring the
already-existing fallback path to run unconditionally instead of behind a
compile-time `#if`. See `samples/Yacht/missing.md`, `samples/SnowShovel/missing.md`.

**Where this pattern does NOT directly apply — audited per-sample (2026-07-05):**
- **AccelerometerSample (#084)** and **TiltPerspective (#107):** unlike Yacht/
  SnowShovel/Bounce, these two samples' original C# ships with **no alternate input
  path at all** — the entire point of each sample is moving a sprite / shifting a 3D
  perspective purely by tilting the phone (confirmed: no `#if WINDOWS_PHONE` split, no
  keyboard/gamepad branch anywhere in `Game.cs`/`AccelerometerHelper.cs`). Porting
  either would mean *inventing* a keyboard-tilt-emulation fallback that doesn't exist
  in the original, which is a bigger deviation than Yacht/SnowShovel/Bounce needed
  (they only had to un-`#if` an existing branch) — but this project already has
  precedent for adding input schemes the original never had at all (DynamicMenu/
  UISample's touch-fallback patterns, NEXT.md §6). **Not a hard blocker — just a
  bigger design decision than usual, worth confirming with the user before doing it**,
  since "invent the missing half of the sample" is a different scope commitment than
  "port what's there."
- **Orientation (#102): this sample has nothing to do with the accelerometer at
  all** — it was miscategorized. It demonstrates `GraphicsDeviceManager
  .SupportedOrientations`/`GameWindow.CurrentOrientation`/`OrientationChanged`
  (screen rotation lock, not a physical sensor reading) — confirmed via full-text
  read of `LayoutSample.cs`/`OrientationSample.cs`: zero references to
  `Accelerometer`/`Compass`/any sensor class anywhere. CNA already has
  `DisplayOrientation.hpp`, and `GraphicsDeviceManager`/`GameWindow` both implement
  `SupportedOrientations`/`CurrentOrientation` (this is the same subsystem behind the
  already-fixed portrait-orientation bug — see the project's own
  `feedback_cna_portrait_orientation_bug` memory). **Likely portable now, pending a
  full read-through to confirm no other blocker** — PLAN.md's "Phone orientation
  sensor" reason for #102 is wrong and should be corrected/re-investigated, not
  treated as settled.
- **GeolocationSample (#095): genuinely unrelated to the accelerometer and still a
  real hard blocker** — uses `System.Device.Location.GeoCoordinateWatcher` (real
  Windows Phone GPS/network location service), with no fallback of any kind in the
  original. This one's "Phone GPS hardware" reason in PLAN.md is accurate as-is.

**Effort:** — (no CNA change needed; this item is a documentation/classification
correction, not an engineering task). Porting #084/#107 is a scope decision, not a
technical blocker. #102 needs a fresh investigation pass, not a CNA change.

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

## 16. Microphone capture (`Microsoft.Xna.Framework.Audio.Microphone`) ✅ RESOLVED

**Corrected 2026-07-06:** this item was added in the same session that wrote
`samples/MicrophoneEcho/missing.md`, based on general SDL3 capability knowledge
rather than an actual check of `cna`'s current source — a live check the same day
found `Microphone` fully implemented (`src/Microsoft/Xna/Framework/Audio/
Microphone.cpp`, 252 lines, `include/.../Audio/{Microphone,MicrophoneState,
NoMicrophoneConnectedException}.hpp`, real tests), merged via `feature/audio` into
`develop` on 2026-07-04 (commits like "wire Microphone::GetData()/GetQueuedBytes()
to the real stream") — **two days before this item was written**. No CNA gap
remains for MicrophoneEcho (#098); update `samples/MicrophoneEcho/missing.md` and
re-attempt the port.

**Effort:** — (done in `cna`)

---

## 17. Multiplayer networking (`Microsoft.Xna.Framework.Net` / `GamerServices.NetworkSession`) ✅ RESOLVED

**Corrected 2026-07-06:** this item was added in the same session that wrote the
NetRumble/ClientServerSample/NetworkPrediction/PeerToPeer placeholders, based on
the C# originals' API usage without actually checking `cna`'s current source for
whether it already implements `GamerServices`/`Net` — a live check the same day
found a full, real implementation: `include/Microsoft/Xna/Framework/Net/{
NetworkSession,NetworkSessionType,NetworkSessionProperties,AvailableNetworkSession,
AvailableNetworkSessionCollection,...}.hpp`, `NetworkSession.cpp` (836 lines),
`GamerServices/{GamerServicesComponent,AvatarRenderer,...}`, real LAN discovery via
`CNA::Internal::Net::ENetDiscoveryService`, and `NetworkSessionType` including
`SystemLink` (exactly what these 4 samples use) — merged via `feature/net` into
`develop` on 2026-07-04, **two days before this item was written**.

No CNA networking gap remains for ClientServerSample (#091), NetworkPrediction
(#100), or PeerToPeer (#103) — update their `missing.md` files and re-attempt
porting. NetRumble (#062) is now only single-blocked, by item 11 (its 4 custom
`.fx` shaders, including the bloom post-process) — update its `missing.md` to drop
the networking half of its "double-blocked" framing.

**Effort:** — (done in `cna`)

---

## 18. Content-pipeline processor extensibility (build-time `ContentProcessor` chaining)

**What is missing:**
CNA's entire asset story is "convert once, offline, with a standalone tool, into a
static runtime JSON/binary format" (`tools/obj2model.py`, `tools/make_font.py`,
`tools/gen_help_png.py`, etc.). There is no pluggable, MSBuild-time,
C#-`ContentProcessor`-style extensibility point — nothing resembling XNA's
`ContentProcessor<TInput,TOutput>` / `ContentProcessorContext.Convert`/`BuildAsset`
chaining model, where a sample can supply its own processor(s) that run as part of
the content build and transform/synthesize data before it ever reaches the runtime
(e.g. baking a reflection cubemap from a flat photo, or flattening/re-deriving model
data in a project-specific way).

This is a different, deeper class of gap than items #6 (static model format
conversion) or #11 (custom shader conversion) — both of those assume an offline tool
already produced a static input file; this item is about the *meta*-capability of a
custom build-time transform pipeline itself.

**Blocked samples:** CustomModelEffect (#053) — its `CustomModelEffectPipeline`
project chains three custom processors (`EnvironmentMappedModelProcessor` →
`EnvironmentMappedMaterialProcessor` → `CubemapProcessor`) to synthesize a 6-face
reflection cubemap from a single flat photo at build time; see
`samples/CustomModelEffect/missing.md` for the full breakdown. No other audited
Phase 3/4 sample needs this — every other custom-`.fx` sample (BloomSample,
NormalMapping, etc.) applies its effect entirely at runtime via an ordinary
`Content.Load<Effect>()` call, with no custom `ContentProcessor` involved.

**Effort:** L — most likely an informal one-off offline preprocessing script (Python
or C++) that performs the same steps against this sample's specific assets and emits
a `.model.json` + cubemap file CNA's runtime can load directly, rather than a general
pluggable pipeline (no second sample has demonstrated a need for genuine
extensibility yet).

---

## Summary Table

| # | Feature | Repo | Effort | Samples blocked |
|---|---|---|---|---|
| 1 | XNB → open format pipeline | all | M/sample | all |
| 2 | SpriteFont loading | cna | L | many | ✅ done |
| 3 | EasyGL: DiffuseColor ignored for VertexPositionColor | cna | S | Primitives3D | ✅ done |
| 4 | EasyGL: Wireframe mode (OpenGL ES has no glPolygonMode) | cna | M | Primitives3D | ✅ done |
| 5 | VertexPositionNormal + lit shader | cna | L | LensFlare, Graphics3D, PickingSample, TrianglePicking, HeightmapCollision, CustomModelClass, InverseKinematics, ChaseCamera, MarbleMaze | ✅ done for Model-based samples (2026-07-06); Primitives3D still needs a texture-less variant or port workaround |
| 6 | Model asset conversion, static geometry (.x/.fbx → .model.json) | tools | M/model | many | CNA itself works |
| 7 | Audio playback | cna | — | — | ✅ done (SDL3_mixer) |
| 8 | SpriteBatch.DrawString / SpriteFont | cna | M | most | ✅ done |
| 9 | Viewport.AspectRatio | cna | S | 0 (workaround) |
| 10 | GamePadButtons direct access | cna | — | 0 (workaround) |
| 11 | Shader conversion (HLSL .fx → GLSL .shader.json) | tools | M/shader | many Phase 3+ | CNA itself works |
| 12 | RenderTarget2D | cna | — | — | ✅ done |
| 13 | Skeletal animation playback (AnimationClip/Keyframe/AnimationPlayer) | cna | L/XL | SkinningSample, SkinnedModelExtensions, CPUSkinning, CustomModelAnimation | not started |
| 14 | TextureCube content loading (`Content.Load<TextureCube>`) | cna | S | RimLighting | not started |
| 15 | Accelerometer/sensor platform reality (documentation correction, not a gap) | docs | — | AccelerometerSample, TiltPerspective (scope decision, not blocked); Orientation (miscategorized, likely portable); Geolocation (still genuinely blocked) | ✅ no CNA change needed |
| 16 | Microphone capture | cna | M | MicrophoneEcho | ✅ done (merged 2026-07-04) |
| 17 | Multiplayer networking (NetworkSession-alike) | cna | L/XL | ClientServerSample, NetworkPrediction, PeerToPeer (NetRumble still needs item 11) | ✅ done (merged 2026-07-04) |
| 18 | Content-pipeline processor extensibility | tools | L | CustomModelEffect | not started |
