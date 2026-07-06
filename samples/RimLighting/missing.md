# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `RimLighting.htm`) so the CNA-side blocker is documented where a future
porting session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's
"Adding a new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/RimLighting_4_0/RimLighting/RimLighting/Game1.cs`.

## Blocker: `Content.Load<TextureCube>` is not implemented

**XNA behaviour:** Loads a static `head` model and renders it with the **stock**
`EnvironmentMapEffect` (`foreach (EnvironmentMapEffect effect in mesh.Effects)`,
`Game1.cs:210`) using a cubemap (`Content.Load<TextureCube>("OutputCube")`,
`Game1.cs:107`) for the rim-lighting reflection. Confirmed via direct source audit:
**zero custom `.fx` files anywhere in this sample** — it needs no shader-pipeline
work at all, unlike the rest of Phase 3.

**CNA port behaviour:** N/A yet (not ported). `EnvironmentMapEffect` is already
implemented in CNA and proven working (see the `easygl_texturecube_*` example/tests),
and `TextureCube`/DDS decoding both work — but `ContentManager.cpp` has no
`TextureCubeTypeReader` registered, so `Content.Load<TextureCube>(...)` throws.

**Root cause:** A single missing `ContentTypeReader` registration, not a missing
runtime feature.

**What's needed in `cna`:** Add a `TextureCubeTypeReader : ContentTypeReader<Graphics::TextureCube>`
to `cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp`'s built-in type
readers (same shape as the existing `Texture2DTypeReader`), reading a `.dds` cubemap
file. Then convert `RimLighting_4_0`'s `OutputCube.dds` and `head.fbx`/`head` model —
model conversion is the already-proven `.model.json` static-mesh pipeline
(DEFERRED.md item 6), no new tooling needed there.

**Tracked in:** DEFERRED.md item #14 (effort S — the smallest remaining Phase 3 gap).
