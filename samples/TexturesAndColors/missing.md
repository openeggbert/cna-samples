# Missing / Differences from XNA 4.0 original

## Custom HLSL Effect (11 techniques) replaced by BasicEffect

**XNA behaviour:** Loads `TexturesAndColors.fx` — a custom HLSL Effect with 11
named techniques (LightingModColorModTexture, LightingOnly, ColorOnly, TextureOnly,
LightingModColor, LightingAddColor, LightingModTexture, LightingAddTexture,
ColorModTexture, ColorAddTexture, LightingAddColorAddTexture,
LightingAddColorModTexture). Pressing Space/gamepad Y cycles through them,
visibly changing how lighting, vertex color, and texture are combined on the mesh.

**CNA port behaviour:** `TexturesAndColorsGame.hpp` uses `BasicEffect` instead
(`sampleMeshes[activeMesh]->Draw(world, view, projection, Color::White)` drives
`GeometricPrimitive::Draw`, `GeometricPrimitive.hpp` line 110). Space still
increments `activeTechnique` (0–10, `TexturesAndColorsGame.hpp` lines 79-80) but
it is never read anywhere, so cycling produces no visible change.

**Root cause:** Not a current CNA limitation — `Content.Load<Effect>()` for
user-authored GLSL shaders (`.shader.json` + hand-translated GLSL) is fully
implemented and working (see the sibling `SpriteEffects` sample port, which
loads four custom effects this way). This sample's port simply did not do the
work of hand-translating the 11-technique HLSL effect to GLSL; it substituted
`BasicEffect` instead.

**Tracked in:** DEFERRED.md item 11 (custom effect shader conversion — CNA's
own capability is marked ✅ resolved; converting this specific 11-technique
shader to GLSL is unstarted work, effort ~L given the technique count).

---

## 3D Models replaced by GeometricPrimitive shapes

**XNA behaviour:** Loads five `.xnb`-compiled models built from `Cube.fbx`,
`SphereHighPoly.fbx`, `SphereLowPoly.fbx`, `Cylinder.fbx`, `Cone.fbx`. Tab/gamepad
X cycles through them.

**CNA port behaviour:** Five `GeometricPrimitive` shapes (this sample's own
local helper classes, in the style of the XNA "Primitives3D" sample) are used as
stand-ins instead of loading the original meshes: CubePrimitive,
SpherePrimitive(tessellation=16), SpherePrimitive(tessellation=8),
CylinderPrimitive, TorusPrimitive. Torus is used in place of Cone because no
`ConePrimitive` shape exists in this codebase (the upstream XNA "Primitives3D"
sample, the usual source for these helper classes, never shipped a cone shape
either — only cube/sphere/cylinder/torus/teapot/bezier).

**Root cause:** Not a current CNA engine limitation for these particular models
— `Content.Load<Model>()` for static, single-bone geometry (which these five
simple shapes are) is implemented end-to-end via `.model.json` +
`tools/obj2model.py` / `tools/fbx_ascii2model.py` (proven working by the
CameraShake and PerformanceMeasuring ports). The gap here is that the five
`.fbx` source models were never converted to `.model.json`, and no cone-shaped
`GeometricPrimitive` helper has been authored anywhere in this codebase yet.

**Tracked in:** DEFERRED.md item 6 (model asset conversion — CNA's runtime is
✅ resolved for single-bone static models; conversion of these five specific
`.fbx` files is unstarted).

---

## Clouds.png texture never loaded or applied to meshes

**XNA behaviour:** `Clouds.png` is loaded as `Texture2D` and passed to the custom
Effect as the `modelTexture` parameter, providing cloud-pattern texturing on
all 3D shapes regardless of the active technique.

**CNA port behaviour:** `Content/Clouds.png` ships in the port's `Content/`
directory but is never loaded (no `Content.Load<Texture2D>` call for it appears
anywhere in `TexturesAndColorsGame.hpp`) or bound to `BasicEffect`. Meshes
render as flat white because `GeometricPrimitive::InitializePrimitive()`
(`GeometricPrimitive.hpp` lines 63-73) builds a `VertexPositionColor` buffer
with every vertex hardcoded to `Color::White` and stores no UV coordinates.

**Root cause:** Not a current CNA rendering limitation — the EasyGL backend
already has a working unlit-textured 3D shader path: `VertexPositionTexture`
(stride-20 vertex buffers) routes to `EnsureTextured3DProgram()`
(`EasyGLGraphicsBackend.cpp` lines 2089-2137), and `BasicEffect` already exposes
`TextureEnabled`/`Texture` setters. This sample's local `GeometricPrimitive`
helper class simply never stores UV data or a texture reference, so there is
nothing to bind — a port-side simplification, not an engine gap.

**Tracked in:** Not a DEFERRED.md item; would require reworking this sample's
local `GeometricPrimitive.hpp` to a UV-carrying vertex format and loading/
binding `Clouds.png` on `BasicEffect` — no CNA engine changes needed.

---

## Flat shading instead of per-vertex lit shading

**XNA behaviour:** The custom effect's lighting techniques compute per-pixel
Lambertian lighting from a vertex normal, `lightDirection`, `lightColor`, and
`ambientColor` (all recomputed every frame in `Update()`/`Draw()`).

**CNA port behaviour:** `diffuseLightDirection`, `diffuseLightColor`, and
`ambientLightColor` are computed every frame in `Update()`
(`TexturesAndColorsGame.hpp` lines 157-168) but are **never passed to
anything** — `BasicEffect` in this port never receives a light direction or
color, and the mesh is always flat white regardless of that (unused)
computation.

**Root cause:** Not a current CNA engine limitation. DEFERRED.md item 3 (the
`VertexPositionColor` shader ignoring `BasicEffect.DiffuseColor`) is already
✅ resolved, and the EasyGL backend additionally already has a working per-pixel
lit+textured shader for `VertexPositionNormalTexture`: stride-32 vertex buffers
route to `EnsureLit3DProgram()` (`EasyGLGraphicsBackend.cpp` lines 2191-2253),
which computes `NdotL` from a vertex normal against `BasicEffect`'s
`DirectionalLight0`/`AmbientLightColor` (both uploaded unconditionally by
`BasicEffect.cpp` lines 36-43, regardless of `LightingEnabled`). This sample's
`GeometricPrimitive` subclasses already compute a per-vertex `normals` array
(via `AddVertex(position, normal)`), but `InitializePrimitive()` discards it,
uploading only position + hardcoded white color. Wiring true lighting would
mean switching this sample's `GeometricPrimitive` to `VertexPositionNormalTexture`
and forwarding the already-computed normals — a port-side fix, not a CNA
blocker.

**Tracked in:** Not a DEFERRED.md item for this specific gap. (DEFERRED.md
item 5 is about a still-missing dedicated `VertexPositionNormal` — without a
texture channel — type/stride; the 32-byte `VertexPositionNormalTexture` lit
path used above already exists and works today.)

---

## No HUD text showing the active technique name

**XNA behaviour:** `SpriteBatch.DrawString` renders the active technique name
(e.g. "LightingModColorModTexture") in the top-left corner every frame.

**CNA port behaviour:** No text overlay of any kind is drawn (only the F1 help
texture).

**Root cause:** Not a current CNA limitation — `SpriteFont` loading and
`SpriteBatch.DrawString` are both fully implemented (DEFERRED.md items 2 and 8,
✅ resolved; see `tools/make_font.py` and its use in the SafeArea/InputSequence
samples). This port simply never added a `SpriteFont` asset or a `DrawString`
call for the technique name — an omission, not a CNA gap. (Moot in practice
today anyway, since `activeTechnique` has no visible effect either way — see
the first section above.)

**Tracked in:** Not a DEFERRED.md item; no CNA change needed.

---

## Gamepad X/Y buttons for cycling mesh/technique are not implemented

**XNA behaviour:** `HandleInput()` (`TexturesAndColors.cs` lines 207-230) checks
gamepad `Buttons.X` (cycle mesh) and `Buttons.Y` (cycle technique) in addition to
keyboard Tab/Space. The sample's own `.htm` documentation's "Sample Controls"
table explicitly lists gamepad **X** = "Display a different 3D primitive" and
**Y** = "Use a different technique to render the current 3D primitive" — and
this same table is what `Content/help.png` (the F1 overlay) was generated from.

**CNA port behaviour:** `HandleInput()` (`TexturesAndColorsGame.hpp` lines
67-92) only checks `Keys::Tab` and `Keys::Space`; `Buttons::X` and `Buttons::Y`
are never read anywhere in the port. The F1 help overlay still advertises
gamepad X/Y as working controls, but pressing them does nothing.

**Root cause:** Omission in the port — `GamePadState::IsButtonDown(Buttons::X)`
/ `IsButtonDown(Buttons::Y)` are already available and used elsewhere in this
same sample (`SampleCamera.hpp` reads `Buttons::A`/`Buttons::B` for zoom), so
adding the equivalent for X/Y would be straightforward; not a CNA gap.

**Tracked in:** Not a DEFERRED.md item; no CNA change needed.
