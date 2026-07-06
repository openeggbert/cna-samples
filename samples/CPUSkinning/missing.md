# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up so the CNA-side
blockers are documented in the same place a future porting session will look. No
`src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's "Adding a new sample" steps for
what's still needed once the CNA gaps below are fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/CPUSkinningSample_4_0/{CpuSkinningDataTypes/
(CpuSkinnedModel.cs, CpuSkinnedModelPart.cs, CpuSkinnedModelReader.cs, CpuVertex.cs,
CpuSkinningHelpers.cs, Animation/(AnimationClip.cs, AnimationPlayer.cs, Keyframe.cs,
SkinningData.cs)), CpuSkinningPipelineExtensions/(CpuSkinnedModelWriter.cs,
CpuSkinnedModelProcessor.cs, GpuSkinnedModelProcessor.cs,
CpuSkinningContentData.cs, SkinningHelpers.cs), CpuSkinningDemo/CpuSkinningDemo/
CpuSkinningDemoGame.cs}`.

## No verbatim `.htm` documentation file
**XNA behaviour:** Most XNA samples ship a `SampleName.htm` documentation file that
is copied verbatim and whose "Sample Controls" table drives the F1 help overlay.
**CNA port behaviour:** This sample ships only `Cpu Skinning Demo Description.doc`
(a Word document) — confirmed by listing every file under
`CPUSkinningSample_4_0/` — so there is no `.htm` to copy verbatim, and (per
CLAUDE.md's instructions for this placeholder pass) none has been authored here
either, since this directory isn't being ported yet. A future porting session should
follow the precedent set by `samples/CatapultWars/missing.md` and
`samples/HoneycombRush/missing.md` (both also sourced from a `.doc`-only kit) and
hand-author a `CPUSkinning.htm` once the sample is actually implemented.
**Root cause:** Packaging difference in the original sample kit.
**Tracked in:** CLAUDE.md "Sample documentation" — documented deviation, to be
resolved when this sample is actually ported.

## Blocker: no `AnimationClip`/`Keyframe`/`AnimationPlayer`/`SkinningData` equivalent, and no per-vertex bone weights in `.model.json`
**XNA behaviour:** `CpuSkinningDemoGame.LoadContent()` loads *two* versions of the
same character and toggles between them (tap / right-click):
```csharp
gpuDude = Content.Load<Model>("dude_gpu");
cpuDude = Content.Load<CpuSkinnedModel>("dude_cpu");
animationPlayer = new AnimationPlayer(cpuDude.SkinningData);
AnimationClip clip = cpuDude.SkinningData.AnimationClips["Take 001"];
animationPlayer.StartClip(clip);
```
Both paths depend on the same `AnimationClip`/`Keyframe`/`AnimationPlayer`/
`SkinningData` types as `SkinningSample` (this sample's own copies live under
`CpuSkinningDataTypes/Animation/`, byte-for-byte the same helper classes) to decode
`animationPlayer.SkinTransforms` each frame, which then feeds *either*:
- `Draw()`'s GPU path: `SkinnedEffect.SetBoneTransforms(animationPlayer.SkinTransforms)`
  on `gpuDude`'s meshes (exactly `SkinningSample`'s technique), or
- `Draw()`'s CPU path: `modelPart.SetBones(animationPlayer.SkinTransforms)` for each
  `CpuSkinnedModelPart` of `cpuDude`, which (`CpuSkinnedModelPart.SetBones`, in
  `CpuSkinnedModelPart.cs`) calls `CpuSkinningHelpers.SkinVertex(bones, ref
  position, ref normal, ref blendIndices, ref blendWeights, out skinnedPosition, out
  skinnedNormal)` **per vertex, on the CPU, every frame**, writing the results into
  a `DynamicVertexBuffer` of plain `VertexPositionNormalTexture` and rendering with
  an ordinary `BasicEffect` (no `SkinnedEffect`/GPU bone-matrix palette involved at
  all for this path).

**CNA port behaviour:** N/A yet (not ported). Same root gap as `SkinningSample`: no
`AnimationClip`/`Keyframe`/`AnimationPlayer`/`SkinningData` types exist anywhere in
`cna/include` or `cna/src` (confirmed by grep — zero matches), and `.model.json`'s
`ModelTypeReader` (`cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`)
carries no per-vertex bone weights/indices or keyframe/clip data to decode in the
first place — see `SkinningSample/missing.md` for the detailed citations (including
why CNA's existing but unrelated `SkinnedModelEXT`/`.skinnedmodel.json`
Avatar-rendering extension doesn't help).

**Root cause:** Real, unimplemented CNA engine capability (skeletal animation
playback) — see DEFERRED.md item #13.

**Tracked in:** DEFERRED.md item #13

## Additional blocker: custom build-time content-pipeline processor (`dude_cpu`/`CpuSkinnedModel`)
**XNA behaviour:** Unlike every other sample in this group, the CPU-skinned half of
this demo is not a plain `Content.Load<Model>` at all. It's a fully custom content
type produced by a dedicated XNA Content Pipeline extension project,
`CpuSkinningPipelineExtensions/`:
- `CpuSkinnedModelProcessor.cs` (`ContentProcessor<NodeContent, CpuSkinnedModelContent>`)
  walks the imported FBX (`dude_cpu.fbx`) at **build time**, reorders vertex/index
  data, and calls `SkinningHelpers.GetSkinningData(input, context, int.MaxValue)` to
  bake bind pose/inverse-bind-pose/skeleton hierarchy/animation clips (CPU skinning
  supports an unbounded bone count, unlike the 59-bone-matrix-palette limit GPU
  skinning has via shader constants — hence the separate `GpuSkinnedModelProcessor.cs`
  used for `dude_gpu.fbx`, which is otherwise the ordinary `SkinnedModelProcessor`
  path `SkinningSample` uses).
- `CpuSkinnedModelWriter.cs` serializes the resulting `CpuSkinningContentData`
  (per-vertex `BLENDINDICES`/`BLENDWEIGHT`-derived `CpuVertex` structs, an index
  buffer, and the same `SkinningData` shape as the GPU path) into the compiled
  `.xnb`.
- At runtime, `CpuSkinnedModelReader.cs` (`ContentTypeReader<CpuSkinnedModel>`)
  deserializes that back into a `CpuSkinnedModel`/`CpuSkinnedModelPart[]` — plain
  data plus a shared `BasicEffect` — with **no XNA `Model`/`ModelMesh` involved on
  the CPU-skinned side at all**.

**CNA port behaviour:** N/A yet (not ported). CNA has no content-pipeline / build-time
content-processor concept whatsoever — `.model.json` is hand- or tool-generated
ahead of time and loaded as-is by `ModelTypeReader`; there is no equivalent of
`ContentProcessor<TInput, TOutput>`/`ContentTypeWriter`/custom `.xnb` type
registration anywhere in CNA. This means `CpuSkinnedModelProcessor`/
`CpuSkinnedModelWriter`/`GpuSkinnedModelProcessor` cannot be ported 1:1 no matter
what happens with item #13 — there is no pipeline stage for them to plug into.
**Root cause:** CNA has deliberately never implemented an offline
processor/pipeline architecture (assets are converted by standalone `tools/`
scripts into final runtime formats instead — see CLAUDE.md's "Assets" section).
**What a future port would most likely need to do instead:** once item #13's
`.model.json` animation/bone-weight schema extension and `AnimationPlayer`
equivalent exist, reimplement the CPU-skinning *technique* directly against that
extended runtime format — i.e. write a CNA-native "skin N vertices on the CPU into
a dynamic vertex buffer, render with an unlit/BasicEffect-equivalent shader" helper
that reads the same per-vertex bone weights/indices the GPU `SkinnedEffect` path
would use, rather than porting the C# build-time processor classes themselves. This
is why this sample is additionally more work than `SkinningSample`/
`SkinnedModelExtensions`/`CustomModelAnimation`, all of which only need item #13's
runtime side.
**Tracked in:** DEFERRED.md item #13 (runtime prerequisite); no separate DEFERRED
item exists for "content-pipeline architecture" itself, since CNA's asset-conversion
philosophy (CLAUDE.md "Assets") is a deliberate, permanent design choice rather than
a gap to be filled — not planned as a general feature, evaluate case-by-case per
sample instead.
