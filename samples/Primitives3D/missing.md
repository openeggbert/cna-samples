# Missing / Differences from XNA 4.0 original

## Color change (B key) has no visual effect

**XNA behaviour:** Pressing B cycles the tint color through Red → Green → Blue →
White → Black.  The selected color is passed to `BasicEffect.DiffuseColor` and the
`BasicEffect` lit shader multiplies fragment output by the diffuse color, so the
spinning primitive visibly changes color.

**CNA port behaviour:** Pressing B increments `currentColorIndex` correctly, and
`setDiffuseColorProperty()` is called on the `BasicEffect`.  However the rendered
primitive always appears white regardless of the selected color.

**Root cause:** CNA's EasyGL backend selects the GLSL shader program purely by
vertex stride.  `VertexPositionColor` has stride 16 → `prog_colored_` shader
(`FragColor = vColor`).  This shader has **no `uDiffuseColor` uniform**, so
`loc_diffuse = -1` and `BindDrawParams` never uploads `DiffuseColor` to the GPU.
All vertices carry `Color::White` (set in `GeometricPrimitive::InitializePrimitive`)
and the shader outputs white for every primitive.

**Tracked in:** DEFERRED.md item 3 — fix requires adding `uDiffuseColor` to the
colored 3D GLSL shader in `EasyGLGraphicsBackend.cpp::EnsureColored3DProgram()`.

---

## Wireframe toggle (Y key) has no visual effect

**XNA behaviour:** Pressing Y switches between solid and wireframe rendering by
setting `GraphicsDevice.RasterizerState` to a state with `FillMode.WireFrame` /
`CullMode.None`.  The mesh is drawn as an edge-only outline.

**CNA port behaviour:** Pressing Y toggles `isWireframe` and calls
`device.setRasterizerStateProperty(wireFrameState)`, but the primitive continues
to render solid — there is no visible difference.

**Root cause:** OpenGL ES 3.x does not expose `glPolygonMode`, which is the only
standard way to enable wireframe rendering in OpenGL.  The EasyGL backend explicitly
ignores `FillMode::WireFrame` with a comment
`// FillMode::WireFrame not supported in OpenGL ES — silently ignored`.

**Tracked in:** DEFERRED.md item 4 — workaround would require emulating wireframe
by rewriting triangle indices as line segments at draw time in the EasyGL backend.

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
