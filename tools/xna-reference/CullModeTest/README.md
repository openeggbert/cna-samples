# CullModeTest — minimal XNA 4.0 RasterizerState.CullMode reproducer

Companion to CNA's `examples/rasterizerstate_cullmode_camera_test.cpp` (in the sibling
`cna_graphics` repository) — same vertex data, same camera, same projection. Built to establish
authoritative XNA 4.0 `CullMode` behavior for the culling-compatibility investigation described in
`cna_graphics/docs/xna_culling_compatibility_audit.md`.

## What it does

Draws two flat-colored triangles of opposite winding — a RED one and a GREEN one — four times, in
four screen quadrants, each quadrant using a different `RasterizerState.CullMode`:

| Quadrant     | CullMode                                            |
|--------------|------------------------------------------------------|
| top-left     | `CullMode.None` (both triangles always visible)       |
| top-right    | `CullMode.CullClockwiseFace`                          |
| bottom-left  | `CullMode.CullCounterClockwiseFace` (XNA's documented default) |
| bottom-right | *(RasterizerState never set at all — whatever the real default actually is)* |

The bottom-left and bottom-right quadrants **must look identical** if XNA's real default
`RasterizerState` genuinely has `CullMode = CullCounterClockwiseFace`.

No content pipeline, no textures, no models — matches the audit's own "minimal reproducer"
requirement exactly (Phase 1).

## How to run it (Windows 7 / XNA Game Studio 4.0 VM)

1. Copy this whole `CullModeTest` folder onto the Windows machine.
2. Open `CullModeTest.sln` in Visual Studio (2010, with XNA Game Studio 4.0 installed).
3. Build (Debug|x86) and run (F5).
4. Take a screenshot of the running window (all 4 quadrants visible at once).
5. Compare directly against the equivalent CNA screenshot from
   `cna_test_easygl_rasterizerstate_cullmode_camera` / the Vulkan/Bgfx equivalents (or against
   `docs/xna_culling_compatibility_audit.md`'s own recorded CNA results) — same 2 triangles, same
   camera, same 4 CullMode values.

A one-line summary is also printed to the Visual Studio **Output** window (Debug.WriteLine) each
time `Draw()` runs, describing the quadrant layout in text.

## Interpreting the result

- If top-left shows **both** RED and GREEN triangles, `CullMode.None` is working.
- Compare top-right vs. bottom-left: exactly one of the two triangles should be visible in each,
  and they should be **complementary** (whichever triangle is hidden in one should be the one
  visible in the other) — confirms `CullClockwiseFace`/`CullCounterClockwiseFace` actually gate
  opposite windings, not the same one twice.
- Bottom-left vs. bottom-right must match exactly (confirms the real default `RasterizerState`).

Report back which triangle (RED = the "CW" one, GREEN = the "CCW" one, per this project's own
established shoelace-formula convention — see `Game1.cs`'s own header comment) is visible in each
quadrant.
