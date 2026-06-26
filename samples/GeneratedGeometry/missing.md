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

**Root cause:** CNA has no Content Pipeline; `Content.Load<Model>` is DEFERRED.

**Tracked in:** DEFERRED.md item 6 (Model loading).

---

## Sky generated at runtime instead of Content Pipeline

**XNA behaviour:** `SkyProcessor.cs` generates a textured cylinder mesh at build
time and packages it as a custom `Sky` content type (Model + Texture2D).

**CNA port behaviour:** Sky cylinder is generated at runtime in `Sky::BuildCylinder()`,
using the same algorithm as `SkyProcessor.cs` (32 segments, texCoordTop=0.1,
texCoordBottom=0.9, center caps).  Sky texture loaded via `Content.Load<Texture2D>`.

**Root cause:** Same as terrain — no Content Pipeline.

**Tracked in:** DEFERRED.md item 6.

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

**CNA port behaviour:** No custom sampler state is applied — CNA may not yet
expose per-sampler wrap modes through the public API.  Minor visual difference
at sky top/bottom seam may be visible.

**Root cause:** CNA sampler state API not fully exposed.

**Tracked in:** Not in DEFERRED.md.
