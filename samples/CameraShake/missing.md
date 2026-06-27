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

## 3D scene renders as white stripe (all CNA backends)
**XNA behaviour:** Camera at (1000,1000,1000) with ground scale 0.1 renders correctly —
DirectX clips triangles whose vertices extend behind the camera and the visible ground
and tank scene is displayed.
**CNA port behaviour:** The scene renders as a white stripe only on both the Vulkan and
EasyGL backends. The ground corner vertex at (6554,0,6554) falls behind the camera
(x+z=13108 exceeds the threshold ≈3000 for camera at (1000,1000,1000)), causing
near-plane clipping artefacts. The z-remap bug in CNA Vulkan shaders
(`pos.z = (pos.z + pos.w) * 0.5`) was removed (fix in CNA repo), but the white stripe
persists regardless of backend.
**Root cause:** CNA near-plane clipping of w<0 vertices does not match DirectX behaviour.
**Tracked in:** CNA issue (affects all backends).
