# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `Skinning.htm`) so the CNA-side blocker is documented in the same place a
future porting session will look. No `src/`/`CMakeLists.txt` exist yet — see
CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA gap
below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/SkinningSample_4_0/{SkinnedModel/
(AnimationClip.cs, AnimationPlayer.cs, Keyframe.cs, SkinningData.cs),
SkinnedModelPipeline/SkinnedModelProcessor.cs, SkinningSample/SkinningSample.cs}`.
This is the canonical XNA "Skinned Model Sample" — the reference implementation
DEFERRED.md item #13 is named after, and the one recommended there to prototype
against first.

## Blocker: no `AnimationClip`/`Keyframe`/`AnimationPlayer`/`SkinningData` equivalent, and no per-vertex bone weights in `.model.json`
**XNA behaviour:** `SkinningSample.cs`'s `LoadContent()` does:
```csharp
currentModel = Content.Load<Model>("dude");
SkinningData skinningData = currentModel.Tag as SkinningData;
animationPlayer = new AnimationPlayer(skinningData);
AnimationClip clip = skinningData.AnimationClips["Take 001"];
animationPlayer.StartClip(clip);
```
Every frame, `Update()` calls `animationPlayer.Update(gameTime.ElapsedGameTime, true,
Matrix.Identity)`, and `Draw()` calls `animationPlayer.GetSkinTransforms()` to get a
`Matrix[]` fed straight into `SkinnedEffect.SetBoneTransforms(bones)` before
`mesh.Draw()`. The four helper types this depends on (all copied verbatim into every
sample that uses this pipeline, `SkinningSample` included):
- `SkinningData` (`SkinnedModel/SkinningData.cs`) — bundles
  `Dictionary<string, AnimationClip> AnimationClips`, `List<Matrix> BindPose`,
  `List<Matrix> InverseBindPose`, `List<int> SkeletonHierarchy`; stored in the
  loaded `Model`'s `Tag` property by the content pipeline's
  `SkinnedModelProcessor`.
- `AnimationClip`/`Keyframe` — a named clip is a `Duration` plus an ordered
  `IList<Keyframe>`, each keyframe holding a bone index, a time offset, and a
  local `Matrix` transform.
- `AnimationPlayer` (`SkinnedModel/AnimationPlayer.cs`) — walks the keyframe list
  up to the current time to rebuild a per-bone local `Matrix[]`
  (`UpdateBoneTransforms`), combines it with `SkeletonHierarchy` into world-space
  transforms (`UpdateWorldTransforms`), then multiplies by `InverseBindPose` to
  produce the final GPU-ready skin matrices (`UpdateSkinTransforms`/
  `GetSkinTransforms`).
- The custom `SkinnedModelProcessor` (build-time, MonoGame/XNA content pipeline)
  is what actually walks the imported FBX's skeleton/animation channels and bakes
  the `SkinningData` + per-vertex `BLENDINDICES`/`BLENDWEIGHT` channels into the
  compiled model — none of this happens at runtime in XNA itself.

**CNA port behaviour:** N/A yet (not ported). Confirmed by reading
`cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`'s `ModelTypeReader`:
it parses a static `"bones"` hierarchy and per-mesh vertex/index buffers only — no
bone-weight/bone-index vertex attributes, no keyframe/clip section, are ever read
from `.model.json`. A grep for `AnimationClip`, `AnimationPlayer`, `Keyframe`, or
`SkinningData` (the exact XNA class names) across `cna/include` and `cna/src`
returns zero matches — these types simply do not exist in CNA today.

CNA does already have a *different*, special-purpose NOXNA extension with a
superficially similar shape:
`cna/include/Microsoft/Xna/Framework/Graphics/SkinnedModelEXT.hpp` defines
`KeyframeEXT`/`BoneTrackEXT`/`AnimationClipEXT`/`SkinnedModelEXT`, loaded via a
`SkinnedModelTypeReader` (`ContentManager.cpp` ~line 644) from a `.skinnedmodel.json`
+ binary skeleton/vertex files. But per that header's own doc comment, it is
"used by `AvatarRenderer::EnableRealRenderingEXT` for real avatar rendering...
deliberately not built on `Model`/`ModelBone`/`ModelMesh`" — it is a standalone
GamerServices Xbox-Avatar-rendering path with its own asset format, not reachable
from `Content.Load<Model>("dude")`, and exposes no `Model.Tag`-style hook. It does
not unblock this sample: `SkinningSample.cs` calls `Content.Load<Model>` and reads
`currentModel.Tag as SkinningData`, and CNA's `ModelTypeReader`/`Model` class have
no equivalent of either the `Tag` payload or the vertex bone-weight data
`SkinnedEffect`'s GLSL shader needs.

Also confirmed: `SkinnedEffect`'s GLSL implementation itself is *not* the blocker
— `cna/src/CNA/Internal/Backends/Vulkan/shaders/skinned3d.{vert,frag}.glsl` (and
the Bgfx/EasyGL backend equivalents) already exist and already consume a
bone-transform array via `SkinnedEffect::SetBoneTransforms`. The gap is entirely
upstream of that: nothing in CNA can currently produce a per-frame bone-transform
array from `.model.json`-sourced animation data, because `.model.json` carries no
animation data and no per-vertex skinning weights, and `AnimationClip`/
`AnimationPlayer`/`SkinningData` (the objects that would decode such data) do not
exist.

**Root cause:** Real, unimplemented CNA engine capability (skeletal animation
playback), not an asset-conversion gap — see DEFERRED.md item #13's distinction
from item #6 (static/rigid model loading, already proven) and item #11 (shader
conversion, also already proven).

**Tracked in:** DEFERRED.md item #13
