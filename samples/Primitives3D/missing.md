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

**XNA behaviour:** `SpriteBatch.DrawString` renders a three-line overlay in the
top-left corner explaining the controls (A = change primitive, B = change color,
Y = toggle wireframe).

**CNA port behaviour:** The overlay is absent.  `SpriteBatch` is created but
`DrawString` is not called because CNA has no `SpriteFont` support yet.

**Tracked in:** DEFERRED.md item 2 (SpriteFont loading) and item 8 (DrawString).

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
