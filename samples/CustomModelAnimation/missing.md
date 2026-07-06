# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `CustomModelRigidAndSkinnedAnimations.htm`) so the CNA-side blocker is
documented in the same place a future porting session will look. No
`src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's "Adding a new sample" steps for
what's still needed once the CNA gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/CustomModelAnimation_4_0/{CustomModelAnimation/
(ModelData.cs, ModelAnimationClip.cs, ModelKeyframe.cs, ModelAnimationPlayerBase.cs,
RigidAnimationPlayer.cs, SkinnedAnimationPlayer.cs, RootAnimationPlayer.cs),
CustomModelAnimationPipeline/(AnimatedModelProcessor.cs, SkinnedModelProcessor.cs),
CustomModelAnimationSample/CustomModelAnimationSample/
CustomModelAnimationSample.cs}`.

Unlike the other three samples in this DEFERRED item #13 group, this sample does
**not** reuse the stock `SkinnedModel`/`AnimationClip`/`AnimationPlayer`/
`SkinningData` names from the Skinned Model Sample — it defines its own
parallel-but-equivalent hierarchy (`ModelData`, `ModelAnimationClip`,
`ModelKeyframe`, abstract `ModelAnimationPlayerBase` with `RigidAnimationPlayer`/
`SkinnedAnimationPlayer`/`RootAnimationPlayer` subclasses) and its own custom
content-pipeline processors. It demonstrates **both** rigid (per-part, one bone per
mesh — the same technique as `SplitScreen`'s `Tank.cs`, but keyframed from an
animation clip instead of driven by code) and true skinned (per-vertex bone-weight)
model animation side by side in one sample, toggled with the A/B gamepad buttons
(or A/B keys).

## Blocker: no `ModelAnimationClip`/`ModelKeyframe`/`SkinnedAnimationPlayer`/skinning-data equivalent, and no per-vertex bone weights in `.model.json`
**XNA behaviour:** `CustomModelAnimationSampleGame.LoadContent()` does, for the
skinned half (the rigid half is structurally identical modulo which player class is
used — see below):
```csharp
skinnedModel = Content.Load<Model>("DudeWalk");
ModelData modelData = skinnedModel.Tag as ModelData;
skinnedClip = modelData.ModelAnimationClips["Take 001"];
skinnedPlayer = new SkinnedAnimationPlayer(modelData.BindPose,
                                           modelData.InverseBindPose,
                                           modelData.SkeletonHierarchy);
skinnedPlayer.StartClip(skinnedClip, 1, TimeSpan.Zero);
```
`SkinnedAnimationPlayer` (`SkinnedAnimationPlayer.cs`) mirrors `AnimationPlayer`'s
algorithm exactly: `SetKeyframe` writes a decoded keyframe's `Matrix` into
`boneTransforms[keyframe.Bone]`, and `OnUpdate` walks `skeletonHierarchy` to build
`worldTransforms` then `skinTransforms = inverseBindPose[bone] * worldTransforms[bone]`
per bone — the same three-stage bone/world/skin pipeline as the stock
`AnimationPlayer`, just spelled with different class/field names and driven through
a shared `ModelAnimationPlayerBase` (`StartClip`/`Update`/keyframe-walking loop,
`ModelAnimationPlayerBase.cs`) that additionally supports a playback rate and a
one-shot-vs-loop `duration` plus a `Completed` event — features `AnimationPlayer`
itself doesn't have. `Draw()`'s `DrawSkinnedModel` feeds the result straight into
`SkinnedEffect.SetBoneTransforms(boneTransforms)`, identical to `SkinningSample`.

The rigid half loads `AnimatedCube` the same way, but with a `RigidAnimationPlayer`
(`RigidAnimationPlayer.cs`) whose `SetKeyframe` just assigns
`boneTransforms[keyframe.Bone] = keyframe.Transform` (no world/skin derivation, no
skeleton walk — same one-transform-per-mesh-part idea `SplitScreen`'s `Tank.cs` uses
programmatically, except here the per-part transforms come from baked
`ModelKeyframe`s instead of hand-written code), and `DrawRigidModel` sets
`effect.World = boneTransforms[mesh.ParentBone.Index] * rootTransform * rigidWorld`
per mesh on an ordinary `BasicEffect` — i.e. it needs the same
per-mesh-parent-bone-index plumbing `SplitScreen/missing.md` already documents as
missing from CNA's `.model.json`/`Model`/`ModelMesh` (no per-mesh parent bone is
ever assigned by CNA's `ModelTypeReader`), **plus** a baked keyframe/clip source for
those per-part transforms instead of code-driven ones. A separate
`RootAnimationPlayer` (`RootAnimationPlayer.cs`, also driving a
`ModelAnimationClip`/keyframe stream, but of plain `Matrix` root transforms with no
per-bone indexing at all) additionally repositions the whole rigid or skinned model
in the world over time (`GetCurrentTransform()`), independent of the per-bone/
per-mesh animation.

Both the rigid and skinned model content are produced by this sample's own content
pipeline (`CustomModelAnimationPipeline/AnimatedModelProcessor.cs` for the rigid
per-part case, `SkinnedModelProcessor.cs` for the skinned case — 282 lines each),
which bakes `ModelData`'s `RootAnimationClips`/`ModelAnimationClips`/`BindPose`/
`InverseBindPose`/`SkeletonHierarchy` into the model's `Tag` at build time, the same
general shape as the stock Skinned Model Sample's pipeline just organized to also
capture rigid per-part animation and a separate root-motion track.

**CNA port behaviour:** N/A yet (not ported). Root gap is the same one documented in
`SkinningSample/missing.md`: no equivalent of `AnimationClip`/`AnimationPlayer`/
`Keyframe`/`SkinningData` (or this sample's renamed equivalents `ModelAnimationClip`/
`ModelKeyframe`/`ModelAnimationPlayerBase`/`ModelData`) exists anywhere in
`cna/include` or `cna/src` (confirmed by grep — zero matches for any of these class
names), and `.model.json`'s `ModelTypeReader`
(`cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`) carries neither
per-vertex bone weights/indices nor any keyframe/clip section, nor (per
`SplitScreen/missing.md`) a real per-mesh parent-bone assignment for the rigid case.
This sample is therefore blocked twice over by the same underlying gap: once for its
skinned half (needs `AnimationPlayer`-equivalent decoding into `SkinnedEffect`,
exactly like `SkinningSample`) and once for its rigid half (needs both a per-mesh
bone hierarchy *and* baked keyframe data driving it, a superset of `SplitScreen`'s
already-documented bone-hierarchy gap). CNA's existing but unrelated
`SkinnedModelEXT`/`.skinnedmodel.json` Avatar-rendering extension doesn't help here
either, for the reasons already given in `SkinningSample/missing.md` (different
asset format, not reachable from `Content.Load<Model>`, no `Tag` equivalent).

Unlike `SkinningSample`/`SkinnedModelExtensions`/`CPUSkinning`, this sample's custom
content-pipeline processors would still need re-authoring (they're this sample's own
`AnimatedModelProcessor`/`SkinnedModelProcessor`, not reused from elsewhere), but —
unlike `CPUSkinning`'s CPU-vertex-skinning writer/reader pair — nothing about them
requires build-time-only functionality; once item #13's `.model.json` schema
extension and `AnimationPlayer` equivalent land, this sample's own asset conversion
is just another instance of the same "FBX skeletal animation → extended
`.model.json`/`.animation.json`" tooling work item #13 already calls out as a
follow-on step, not an additional architecture gap the way `CPUSkinning`'s is.

**Root cause:** Real, unimplemented CNA engine capability (skeletal animation
playback, and the related per-mesh bone-hierarchy gap for the rigid half) — see
DEFERRED.md item #13.

**Tracked in:** DEFERRED.md item #13
