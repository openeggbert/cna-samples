# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `SkinnedModelExtensions.htm`) so the CNA-side blocker is documented in the
same place a future porting session will look. No `src/`/`CMakeLists.txt` exist yet
— see CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA
gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/SkinnedModelExtensions_4_0/{SkinnedModel/
(AnimationClip.cs, AnimationPlayer.cs, Keyframe.cs, SkinningData.cs,
SkinnedSphere.cs), SkinnedModelPipeline/SkinnedModelProcessor.cs,
SkinningSample/(SkinningSample.cs, Primitives3D/*)}`.

This sample is explicitly "Part 2" of `SkinningSample` — its own `.htm` documentation
(§"Extending the Skinned Model Sample") describes it as building further examples of
skinned animation on top of the exact same `SkinnedModel`/`SkinnedModelPipeline`
project pair from the original Skinned Model Sample (`SkinningSample_4_0`), adding:
bone lookup by name via a new `SkinningData.BoneIndices` dictionary, manual per-bone
transform overrides layered on top of the decoded animation (rotating the head/left
upper-arm bones from gamepad/keyboard input), a `SkinnedSphere` collision-bounding
type attached to named bones for hit-testing, and attaching a second, ordinary rigid
model (a baseball bat) to an animated hand bone's world transform. Everything this
sample adds is additive on top of `SkinningSample`'s runtime pipeline, so it inherits
that sample's blocker outright, plus a bit more surface area.

## Blocker: no `AnimationClip`/`Keyframe`/`AnimationPlayer`/`SkinningData` equivalent, and no per-vertex bone weights in `.model.json`
**XNA behaviour:** `SkinningSample.cs`'s `LoadContent()` (identical setup to Part 1)
does:
```csharp
currentModel = Content.Load<Model>("dude");
skinningData = currentModel.Tag as SkinningData;
boneTransforms = new Matrix[skinningData.BindPose.Count];
animationPlayer = new AnimationPlayer(skinningData);
AnimationClip clip = skinningData.AnimationClips["Take 001"];
animationPlayer.StartClip(clip);
skinnedSpheres = Content.Load<SkinnedSphere[]>("CollisionSpheres");
```
Then every frame, `Update()` does what Part 1 didn't need:
```csharp
animationPlayer.UpdateBoneTransforms(gameTime.ElapsedGameTime, true);
animationPlayer.GetBoneTransforms().CopyTo(boneTransforms, 0);

int headIndex = skinningData.BoneIndices["Head"];
int armIndex  = skinningData.BoneIndices["L_UpperArm"];
boneTransforms[headIndex] = headTransform * boneTransforms[headIndex];
boneTransforms[armIndex]  = armTransform  * boneTransforms[armIndex];

animationPlayer.UpdateWorldTransforms(Matrix.Identity, boneTransforms);
animationPlayer.UpdateSkinTransforms();
```
i.e. it decodes the clip's bone-local transforms, *overwrites two of them* with
gamepad/keyboard-driven rotations, then re-derives world and skin transforms from
the edited array — something only possible once `AnimationPlayer`'s three-stage
bone/world/skin pipeline already exists. It also looks up bones by name via
`skinningData.BoneIndices["Head"]` (a dictionary this sample's variant of
`SkinningData` adds beyond the Part 1 version) and uses
`animationPlayer.GetWorldTransforms()` results to place a `SkinnedSphere`
(`BoneName`, `Radius`, `Offset` fields) collision volume per bone, and to attach a
second rigid `Model` (a baseball bat, drawn with plain `BasicEffect`) to the
`"L_Index1"` hand bone's absolute transform.

**CNA port behaviour:** N/A yet (not ported). Same root gap as `SkinningSample`: no
`AnimationClip`/`Keyframe`/`AnimationPlayer`/`SkinningData` types exist anywhere in
`cna/include` or `cna/src` (confirmed by grep — zero matches), and `.model.json`
carries no keyframe/clip data or per-vertex bone weights for `ModelTypeReader`
(`cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`) to read in the first
place. This sample additionally needs a `BoneIndices` name-lookup and a
`SkinnedSphere[]`-shaped content type, but both are trivial additions once the core
`AnimationPlayer`/`SkinningData` machinery exists — they are not separate blockers,
just extra surface area on top of the same missing feature.

CNA's existing `SkinnedModelEXT` (`cna/include/Microsoft/Xna/Framework/Graphics/
SkinnedModelEXT.hpp`, loaded via `.skinnedmodel.json` for GamerServices Avatar
rendering — see `SkinningSample/missing.md` for the full citation) doesn't help
here either, for the same reason: it isn't reachable from `Content.Load<Model>`,
has no `Tag`-equivalent, and is a distinct asset format from `.model.json`.

**Root cause:** Real, unimplemented CNA engine capability (skeletal animation
playback) — see DEFERRED.md item #13.

**Tracked in:** DEFERRED.md item #13
