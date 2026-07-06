# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `InstancedModel.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source:
`/rv/tmp/XNAGameStudio/Samples/InstancedModelSample_4_0/InstancedModelSample/InstancedModelSample/InstancedModelSampleGame.cs`
and
`.../InstancedModelPipeline/InstancedModelProcessor.cs`.

## Blocker: custom `InstancedModel.fx` HLSL shader

**XNA behaviour:** A custom content processor points the spinning models'
material at a hand-written effect
(`InstancedModelProcessor.cs:49`: `newMaterial.Effect = new
ExternalReference<EffectContent>("InstancedModel.fx", ...)`). The shader
(`InstancedModelSample/Content/InstancedModel.fx`) defines **two techniques**
that the sample switches between at runtime depending on
`InstancingTechnique`: `HardwareInstancing` (`vs_3_0` — reads a per-instance
world matrix from a secondary vertex stream via a `BLENDWEIGHT`-semantic
parameter, `InstancedModelSampleGame.cs:251-264` sets `CurrentTechnique =
effect.Techniques["HardwareInstancing"]` and calls
`GraphicsDevice.DrawInstancedPrimitives`) and `NoInstancing` (`vs_2_0` — takes
`World` as a plain effect parameter and loops, drawing once per instance,
`InstancedModelSampleGame.cs:288-303`). Both techniques share a simple Lambert
diffuse + ambient lighting pixel shader. Ground/ landscape geometry uses stock
`BasicEffect` and is not blocked.

**CNA port behaviour:** N/A yet (not ported). Note: `GraphicsDevice::DrawInstancedPrimitives`
IS already implemented in CNA (see `cna/src/Microsoft/Xna/Framework/Graphics/GraphicsDevice.cpp`
and the `vulkan_instanced_test.cpp` example) — hardware instancing itself is not
a blocker here, only the custom shader that consumes the per-instance stream is.

**Root cause:** `InstancedModel.fx` (both its `vs_3_0` hardware-instancing
vertex shader and its `vs_2_0` no-instancing vertex shader, sharing one pixel
shader) is genuine custom HLSL that must be hand-translated to GLSL plus a
`.shader.json` descriptor before `Content.Load<Effect>` (already implemented
in CNA for `.shader.json`-described effects) can serve it.

**Tracked in:** DEFERRED.md item #11.
