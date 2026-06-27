# Missing / Differences from XNA 4.0 original

## VibrationManager omitted
**XNA behaviour:** A `VibrationManager` GameComponent vibrates the gamepad motors
(left/right, with linear decay) in sync with the camera shake. On Windows Phone it
also triggers the phone's vibration motor via `VibrateController`.
**CNA port behaviour:** No vibration — the camera shake visual effect is identical
but no rumble feedback is produced.
**Root cause:** `GameComponent`/`Game.Components` not yet in CNA; `GamePad.SetVibration`
exists but is not wired up without the component. Low priority for a desktop demo.
**Tracked in:** not planned

## Touch input removed
**XNA behaviour:** On Windows Phone, tap triggers a short shake and double-tap triggers
a long shake via `TouchPanel`.
**CNA port behaviour:** Desktop only — keyboard A / X keys used instead.
**Root cause:** Touch input is phone-specific.
**Tracked in:** not planned

## Model format converted from FBX/X to .model.json
**XNA behaviour:** Loads `tank.fbx` (ASCII FBX 6.1, 12 mesh parts) and `Ground.x`
via XNA ContentManager.
**CNA port behaviour:** Models converted to CNA's `.model.json` binary format using
`tools/fbx_ascii2model.py` (for FBX) and `tools/obj2model.py` (for .x via assimp).
Geometry and normals are preserved; textures reference original TGA files.
**Root cause:** CNA does not load FBX/X natively; uses its own .model.json format.
**Tracked in:** DEFERRED.md item #6

## Textures not assigned to model meshes
**XNA behaviour:** `BasicEffect` on each mesh is given the appropriate texture
(`engine_diff_tex.tga` for engine/wheel parts, `turret_alt_diff_tex.tga` for turret).
**CNA port behaviour:** Textures are present in Content/ but the `.model.json` format
does not yet specify per-mesh textures — meshes render with `BasicEffect` default
(untextured, white diffuse).
**Root cause:** `.model.json` `"effect"` field does not support texture assignment;
would require a named `.shader.json` Effect or a texture field extension.
**Tracked in:** not planned (visual difference only)

## Ground uses checker texture but renders untextured
**XNA behaviour:** Ground plane uses `SamplerState.LinearWrap` and `Checker.bmp`
tiled texture via BasicEffect.
**CNA port behaviour:** Ground renders as plain white/grey (no texture assigned).
**Root cause:** Same as mesh texture limitation above.
**Tracked in:** not planned

## 3D scene rendering (fixed in CNA Vulkan shaders + ground scale)
**XNA behaviour:** Camera at (1000,1000,1000) with ground scale 0.1 renders correctly.
DirectX clips triangles whose vertices extend behind the camera (near-plane clip of
the +X+Z corner vertex at (6554,0,6554)) and the visible portion of the ground renders.
**CNA port behaviour:** Ground scale reduced from 0.1 to 0.02 to keep all vertices
in front of the camera. At scale 0.02 the farthest ground vertex at (1310,0,1310) has
x+z=2620 which is safely inside the camera forward half-space (threshold ≈ 3000 for
camera at (1000,1000,1000)). Scene renders correctly at the smaller scale.
**Root cause (two issues fixed):**
1. All CNA Vulkan 3D vertex shaders previously applied `pos.z = (pos.z + pos.w) * 0.5`
   (OpenGL→Vulkan z remap), but CNA's `CreatePerspectiveFieldOfView` already uses the
   XNA/DirectX [0,w] clip-space z convention. Removed from all 5 affected shaders.
2. Vulkan near-plane clipping of triangles with w<0 vertices produces visible artifacts
   (white stripe) rather than the clean clip DirectX provides. Reducing ground scale
   eliminates any vertex behind the camera, avoiding the clip entirely.
**Tracked in:** Shader fix in CNA `src/CNA/Internal/Backends/Vulkan/shaders/`; ground
scale difference is a CNA/Vulkan near-plane clipping limitation.
