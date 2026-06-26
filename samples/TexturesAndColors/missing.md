# Missing / Differences from XNA 4.0 original

## Custom HLSL Effect replaced by BasicEffect

**XNA behaviour:** Loads `TexturesAndColors.fx` — a custom HLSL Effect with 11
named techniques (LightingModColorModTexture, LightingOnly, ColorOnly, TextureOnly,
LightingModColor, LightingAddColor, LightingModTexture, LightingAddTexture,
ColorModTexture, ColorAddTexture, LightingAddColorAddTexture,
LightingAddColorModTexture).  Pressing Space cycles through them, visibly changing
how lighting, vertex color, and texture are combined on the mesh.

**CNA port behaviour:** Custom Effect loading is not supported.  The port uses
`BasicEffect` instead.  Space still increments `activeTechnique` (0–10) but
produces no visible change.

**Root cause:** CNA has no HLSL-to-GLSL transpiler and no `Content.Load<Effect>`
for user-authored shaders.

**Tracked in:** DEFERRED.md — not currently planned; would require an Effect
compiler pipeline.

---

## 3D Models replaced by GeometricPrimitive shapes

**XNA behaviour:** Loads five `.xnb` models: Cube, SphereHighPoly, SphereLowPoly,
Cylinder, Cone.  Tab cycles through them.

**CNA port behaviour:** Five `GeometricPrimitive` shapes are used as stand-ins:
CubePrimitive, SpherePrimitive(tessellation=16), SpherePrimitive(tessellation=8),
CylinderPrimitive, TorusPrimitive (closest available — no ConePrimitive exists).

**Root cause:** `Content.Load<Model>` is not implemented.

**Tracked in:** DEFERRED.md item 6 (Model loading).

---

## Texture not applied to meshes

**XNA behaviour:** `Clouds.png` is loaded as `Texture2D` and passed to the custom
Effect as the `modelTexture` parameter, providing cloud-pattern texturing on
the 3D shapes.

**CNA port behaviour:** The port does not load or apply the texture.  Meshes render
in flat white (vertex color) because `GeometricPrimitive` uses `VertexPositionColor`
(no UV coordinates) and `BasicEffect` in vertex-color mode ignores textures.

**Root cause:** Our `GeometricPrimitive` substitute stores no UV data; applying
a texture would require `VertexPositionTexture` or `VertexPositionNormalTexture`
geometry, which in turn requires the normal-lit shader (DEFERRED.md item 5).

---

## No HUD text (active technique name)

**XNA behaviour:** `SpriteBatch.DrawString` renders the active technique name
(e.g. "LightingModColorModTexture") in the top-left corner.

**CNA port behaviour:** No text overlay — CNA has no SpriteFont support yet.

**Tracked in:** DEFERRED.md item 2 (SpriteFont) and item 8 (DrawString).

---

## SampleGrid renders at visual scale without GridScale conversion

**XNA behaviour:** `grid.GridScale = 1.0f; grid.GridSize = 32;` produces 32 grid
squares at 1-unit spacing — a 32×32 unit ground plane.

**CNA port behaviour:** Same values; grid renders correctly as a 32×32 flat line
grid in the XZ plane.  No functional difference.

---

## Flat shading instead of lit shading

Same issue as Primitives3D: `GeometricPrimitive` uses `VertexPositionColor` (white
vertices), so the BasicEffect colored shader has no `uDiffuseColor` uniform and the
mesh always renders white regardless of lighting settings.

**Tracked in:** DEFERRED.md item 3 (DiffuseColor in colored shader) and
item 5 (VertexPositionNormal + lit shader).
