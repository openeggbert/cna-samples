# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `Ship_Game_Starter_Kit.htm`) so the CNA-side blocker is documented in the
same place a future porting session will look. No `src/`/`CMakeLists.txt` exist yet
— see CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA
gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/ShipGame_4_0/ShipGame/`.

## Blocker: Custom HLSL `.fx` shaders
**XNA behaviour:** ShipGame renders animated sprites, normal-mapped ship models,
a post-process blur, and particle effects using four custom HLSL pixel/vertex
shaders:
```
Content/shaders/AnimSprite.fx
Content/shaders/NormalMapping.fx
Content/shaders/Blur.fx
Content/shaders/Particle.fx
```

**CNA port behaviour:** N/A yet (not ported). Same gap as BloomSample,
NormalMapping, and the rest of DEFERRED.md item #11's list: HLSL `.fx` shaders must
be rewritten as GLSL and described via `.shader.json` before
`Content.Load<Effect>()` can use them.

**Root cause:** HLSL→GLSL shader conversion has not been done for any of these 4
effects.

**Tracked in:** DEFERRED.md item #11.

## Cross-reference: this sample has a direct MonoGame.Samples equivalent
Unlike every other sample in this repo's whole catalog, ShipGame has a maintained,
directly-equivalent port already available at `/rv/tmp/MonoGame.Samples/ShipGame/`
(the official `MonoGame/MonoGame.Samples` repo, already referenced elsewhere in this
repo's `PLAN.md`). Once the shader-conversion blocker above is resolved, that
MonoGame port is a useful cross-check for expected visual/gameplay behavior
(confirming shader output, ship movement/normal-mapping look, and particle/blur
effects match) since it targets the same original XNA sample rather than being an
independent reimplementation.

**Tracked in:** DEFERRED.md item #11.
