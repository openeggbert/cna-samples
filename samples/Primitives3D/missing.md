# Missing / Differences from XNA 4.0 original

## Color change (B key) has no visual effect — RESOLVED

**Status:** Fixed in CNA (EasyGL backend).  Pressing B now visibly cycles the tint
Red → Green → Blue → White → Black, matching XNA.

**Was:** The colored 3D shader selected for `VertexPositionColor` (stride 16) output
`FragColor = vColor` with no `uDiffuseColor` uniform, so `BindDrawParams` never
uploaded `DiffuseColor`; every vertex carries `Color::White`, so the primitive always
rendered white.

**Fix:** `EnsureColored3DProgram()` now declares `uniform vec4 uDiffuseColor` and the
fragment shader outputs `vColor * uDiffuseColor`; `prog_colored_.loc_diffuse` is wired
to the uniform.  Default `diffuseColor` is `{1,1,1,1}`, so callers that set no diffuse
are unaffected.  (`cna/.../EasyGL/EasyGLGraphicsBackend.cpp`.)

**Tracked in:** DEFERRED.md item 3 (resolved).

---

## Wireframe toggle (Y key) has no visual effect — RESOLVED

**Status:** Fixed in CNA (EasyGL backend).  Pressing Y now switches the primitive
between solid and an edge-only wireframe, matching XNA `FillMode.WireFrame`.

**Was:** OpenGL ES 3.x has no `glPolygonMode`, so the EasyGL backend silently ignored
`FillMode::WireFrame` and always drew solid.

**Fix:** `ApplyRasterizerState` records a `wireframe_` flag when `FillMode::WireFrame`
is set; the 3D draw paths then re-expand each triangle into `GL_LINES` (via the new
`DrawWireframe` helper) instead of `GL_TRIANGLES`, drawn through a scratch 32-bit line
index buffer.  Covers both indexed and non-indexed triangle list/strip draws.
(`cna/.../EasyGL/EasyGLGraphicsBackend.cpp`.)

**Tracked in:** DEFERRED.md item 4 (resolved).

---

## No HUD text (controls overlay)

**XNA behaviour:** `Primitives3DGame.cs` loads `Content.Load<SpriteFont>("hudfont")`
and each `Draw()` renders a three-line overlay in the top-left corner explaining the
controls ("A or tap top of screen = Change primitive", "B or tap bottom left of
screen = Change color", "Y or tap bottom right of screen = Toggle wireframe") via
`spriteBatch.DrawString(spriteFont, text, new Vector2(48, 48), Color.White)`.

**CNA port behaviour:** The overlay is absent. `Primitives3DGame.hpp` constructs a
`spriteBatch` member in `LoadContent()` (line 131) but never loads a `SpriteFont` or
calls `DrawString` with it anywhere in `Draw()` — the member is otherwise unused.

**Root cause:** Not a current CNA limitation — `SpriteFont` loading and
`SpriteBatch.DrawString` are both fully implemented in CNA now (see DEFERRED.md
items 2 and 8, both ✅ resolved; already used for on-screen text in
`samples/SafeArea` and `samples/InputSequence`). This is simply an unported piece of
the sample: no `.font.json`/atlas font asset was generated for Primitives3D's
`Content/`, and the corresponding `DrawString` call was never added. Porting it would
require `tools/make_font.py` to generate a font asset and adding the `DrawString`
call to `Draw()`.

**Tracked in:** not planned (port gap, not a CNA gap) — DEFERRED.md items 2 and 8 are
resolved and no longer block this.

---

## Flat shading instead of lit shading

**XNA behaviour:** `BasicEffect.EnableDefaultLighting()` activates a directional
light.  Vertices carry position + normal (`VertexPositionNormal`), and the shader
computes per-fragment lighting (diffuse + specular), giving the primitives a 3D
shaded appearance.

**CNA port behaviour:** `VertexPositionNormal` is not supported by CNA.  The port
uses `VertexPositionColor` with white vertex colors.  The colored 3D shader applies
no lighting — all faces are rendered at the same brightness (flat white).

**Tracked in:** DEFERRED.md item 5 — requires adding `VertexPositionNormal` vertex
type and a normal-lit GLSL shader to the EasyGL backend.
