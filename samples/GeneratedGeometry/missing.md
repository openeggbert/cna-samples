# Missing / Differences from XNA 4.0 original

## Terrain generated at runtime instead of Content Pipeline

**XNA behaviour:** `TerrainProcessor.cs` (a custom `ContentProcessor`) runs during
the build and converts `terrain.bmp` (heightmap) into a compiled `.xnb` Model with
smooth normals and `rocks.bmp` texture.  The game loads it with `Content.Load<Model>`.

**CNA port behaviour:** The terrain is generated at runtime in `Terrain::Build()`,
replicating the exact same heightmap-to-mesh algorithm:
`position.Y = (height - 1) * 64`, two triangles per quad,
UV = `(x, y) * 0.1`, gradient-based normals.  `rocks.bmp` is loaded as
`Texture2D` via `Content.Load<Texture2D>` and applied directly to `BasicEffect`.

**Root cause:** `Content.Load<Model>()` itself IS supported by CNA for static
models (`.model.json` + binary vertex/index files — DEFERRED.md item 6, now
marked "✅ CNA supports static models"), but that only covers loading an
*already-built* model file. The actual gap is that CNA has no build-time,
pluggable `ContentProcessor<TInput,TOutput>`-style extensibility (DEFERRED.md
item 18) — there is no way to write a `TerrainProcessor` equivalent that runs
during asset build and programmatically synthesizes a mesh from a heightmap
texture. Generating the mesh at runtime in C++ is the pragmatic substitute.

**Tracked in:** DEFERRED.md item 18 (content-pipeline processor extensibility).

---

## Sky generated at runtime instead of Content Pipeline

**XNA behaviour:** `SkyProcessor.cs` generates a textured cylinder mesh at build
time and packages it as a custom `Sky` content type (Model + Texture2D).

**CNA port behaviour:** Sky cylinder is generated at runtime in `Sky::BuildCylinder()`,
using the same algorithm as `SkyProcessor.cs` (32 segments, texCoordTop=0.1,
texCoordBottom=0.9, center caps).  Sky texture loaded via `Content.Load<Texture2D>`.

**Root cause:** Same as terrain — no build-time custom `ContentProcessor`
extensibility point (DEFERRED.md item 18), not a `Content.Load<Model>` gap.

**Tracked in:** DEFERRED.md item 18.

---

## Specular lighting silently ignored

**XNA behaviour:** `effect.SpecularColor = new Vector3(0.6, 0.4, 0.2)` and
`effect.SpecularPower = 8` add warm specular highlights to the terrain.

**CNA port behaviour:** `setSpecularColorProperty` and `setSpecularPowerProperty`
are called, but the EasyGL `prog_lit_textured_` GLSL shader has no specular term —
only diffuse + ambient + texture.  Specular highlights are absent.

**Root cause:** EasyGL lit shader does not implement Blinn-Phong specular.

**Tracked in:** Not in DEFERRED.md; add if specular becomes important.

---

## Fog uses object-space Z instead of camera-space depth

**XNA behaviour:** `BasicEffect` fog uses camera-space depth (distance from
camera along view axis) which gives correct radial fog around the camera.

**CNA port behaviour:** EasyGL `prog_lit_textured_` fog uses `aPos.z`
(object/world-space Z coordinate), not camera-space depth.  Fog distribution
will differ from XNA — heavier on the +Z side of the world regardless of
camera direction.

**Root cause:** EasyGL lit shader simplifies fog to world-space Z.

**Tracked in:** Not in DEFERRED.md; minor visual difference.

---

## Custom SamplerState (WrapUClampV) not applied to sky

**XNA behaviour:** Sky uses `SamplerState { AddressU=Wrap, AddressV=Clamp }`
to prevent vertical seams in the sky texture.

**CNA port behaviour:** No custom sampler state is applied to slot 0 before
drawing the sky, so it keeps whichever `SamplerState` was last set (default
`LinearWrap` — wraps both U and V).  Minor visual difference (a seam) may be
visible at the sky top/bottom poles under bilinear filtering.

**Root cause:** Port gap, not a CNA limitation — `SamplerState` (default
constructor + public `setAddressUProperty`/`setAddressVProperty` setters,
`GraphicsDevice::getSamplerStatesProperty()[slot]`) is fully implemented and
is applied to the backend automatically before every draw call
(`GraphicsDevice::applySamplerStatesToBackend()`, called from
`DrawIndexedPrimitives`/`DrawUserPrimitives`/etc.), and the EasyGL backend
(`EasyGLGraphicsBackend::ApplySamplerState`) honours per-axis
Wrap/Clamp/Mirror address modes. The XNA original's private
`SamplerState { AddressU = Wrap, AddressV = Clamp }` object-initializer
pattern isn't directly portable (CNA's parameterized constructor is private,
used only for the built-in presets), but the equivalent
`SamplerState s; s.setAddressUProperty(TextureAddressMode::Wrap); s.setAddressVProperty(TextureAddressMode::Clamp); device.getSamplerStatesProperty()[0] = s;`
would work — `Sky::Draw()` simply never sets it.

**Tracked in:** Not in DEFERRED.md; port gap that could be fixed directly in
`Sky.hpp` without any CNA framework change.

---

## Terrain and sky forced to double-sided rendering (`RasterizerState::CullNone`)

**XNA behaviour:** Neither `GeneratedGeometry.cs`/`DrawTerrain()` nor `Sky.cs`
ever touches `GraphicsDevice.RasterizerState`; both meshes render with
whatever cull state the device already has, which for XNA is the default
`RasterizerState.CullCounterClockwise` (single-sided, back-face culled).

**CNA port behaviour:** `Terrain::Draw()` (`Terrain.hpp:141`) and `Sky::Draw()`
(`Sky.hpp:117`) both explicitly set
`device.setRasterizerStateProperty(RasterizerState::CullNone)` before drawing
and restore `RasterizerState::CullCounterClockwise` afterward. CNA's
`GraphicsDevice` already defaults to `RasterizerState::CullCounterClockwise`
(matching XNA — see `GraphicsDevice.cpp` constructor), so this override was
not needed to match the XNA *default*; it was added to make the
procedurally-generated geometry render at all/correctly. Both the terrain
quads and the sky cylinder use vertex winding copied index-for-index from the
XNA content-pipeline processors (`TerrainProcessor.cs`/`SkyProcessor.cs`), so
this strongly suggests the front/back-face determination for this winding
comes out flipped somewhere in CNA's EasyGL 3D rendering path relative to
XNA/Direct3D's convention (XNA/D3D traditionally treats clockwise-wound
triangles as front-facing; OpenGL's default is counter-clockwise, and the
Y-origin flip between D3D's top-left and OpenGL's bottom-left screen space is
a classic source of this exact mismatch — CNA is aware of this flip elsewhere,
e.g. `EasyGLGraphicsBackend::SetScissorRect`'s comment about converting
top-left XNA coordinates to OpenGL's bottom-left origin). Net effect: both
meshes render double-sided in the CNA port where XNA would cull back faces —
e.g. if the camera moved below the terrain surface or outside the sky
cylinder, the CNA port would still render geometry that XNA would cull away.

**Root cause:** Not confirmed at the CNA-framework level (no test or DEFERRED.md
item covers 3D winding/handedness); the CullNone override looks like a
pragmatic workaround added during porting rather than a deliberate design
choice, since it deviates from both the XNA original and CNA's own default
rasterizer state.

**Tracked in:** Not in DEFERRED.md; not planned unless a winding/handedness
bug is confirmed and fixed at the CNA framework level.
