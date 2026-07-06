# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up so the CNA-side
blocker is documented in the same place a future porting session will look. No
`src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's "Adding a new sample" steps for
what's still needed once the CNA gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/MarbleMaze_4_0/`.

## No `.htm` documentation file exists for this sample
Unlike almost every other XNA 4.0 Phase-6 sample, MarbleMaze ships **no** `SampleName.htm`
anywhere under its source tree (confirmed by a full `find -iname "*.htm*"` over
`MarbleMaze_4_0/`, zero results). Its documentation instead is a 101-page Word
tutorial, `3D Game Development With XNA.doc`, at the top of `MarbleMaze_4_0/` — a
step-by-step book chapter that walks through building the game across the source
tree's `T1_3D Drawing` / `T2_3D Movement & Camera` / `T3_Game Physics` tutorial
stages (see below). Since CLAUDE.md's process copies `SampleName.htm` verbatim and
none exists, this directory intentionally has **no** `.htm` file — do not fabricate
one. If a future session wants sample-controls text for the F1 help overlay, it will
need to be derived from the `.doc` (or from `Program.cs`/`MarbleMazeGame.cs`'s own
input-handling comments) rather than copied from an `.htm`.

## Source tree is a multi-stage tutorial, not a flat sample
`MarbleMaze_4_0/Source/` contains two top-level stages:
- **`EX1_MarbleMazeGame/`** — three tutorial checkpoints (`T1_3D Drawing`,
  `T2_3D Movement & Camera`, `T3_Game Physics`), each with `Begin/`/`End/`
  sub-stages. This is the early, incrementally-built tutorial version of the game
  (basic 3D drawing → camera/movement → physics), analogous to a "part 1/2/3" book
  walkthrough.
- **`EX2_Polishing/`** — the final stage, itself split into `Begin/`/`End/`. `End/`
  is the complete, polished game (adds visual/gameplay polish on top of EX1's
  physics-complete base — e.g. particle effects, audio, UI polish, the marble's
  lit `BasicEffect` rendering used across `Marble.cs`, `Maze.cs`, and
  `DrawableComponent3D.cs`).

This is **not** a duplicate "training kit" situation like HoneycombRushTrainingKit or
CatapultWarsTrainingKit (both excluded in `ignored.md` because they duplicate an
*already-ported different sample*). MarbleMaze is a normal Phase 6 Todo sample; its
`EX1`/`EX2` split is simply how this one sample's source happens to be organized —
build *stages* of the same single game, not a second sample. **A future port should
target `Source/EX2_Polishing/End/`** (the final, fully polished build) as the thing
to port, using `EX1`'s earlier stages only as reference for how the tutorial
introduces each mechanic incrementally, if that's useful context.

## Blocker: lit `BasicEffect` rendering (`VertexPositionNormal` gap)
**XNA behaviour:** The EX2/End build's 3D objects — the marble, the maze geometry,
and the generic `DrawableComponent3D` base class shared by both — render using
`BasicEffect.EnableDefaultLighting()` for normal per-vertex lit shading (a marble
that shows shading as it rolls through the maze, plus lit maze walls/floor).
Confirmed by direct grep of `Source/EX2_Polishing/End/MarbleMazeGame/MarbleMazeGame/`:
```
Objects/DrawableComponent3D.cs:152:    effect.EnableDefaultLighting();
Objects/Marble.cs:158:                effect.EnableDefaultLighting();
Objects/Maze.cs:86:                    effect.EnableDefaultLighting();
```
All three call sites loop `foreach (BasicEffect effect in mesh.Effects)` and call
`effect.EnableDefaultLighting()` — no custom `.fx` shader is involved anywhere in
this sample; the entire renderer is stock `BasicEffect` with lighting turned on.

**CNA port behaviour:** N/A yet (not ported). CNA lacks `VertexPositionNormal` and a
corresponding lit shader path in the EasyGL backend — see DEFERRED.md item #5, which
already lists "MarbleMaze's EX2/End tutorial stage" by name as one of the nine
samples blocked by this exact gap (confirmed by direct source audit, not by
association with the unrelated custom-shader item #11).

**Root cause:** CNA's `VertexPositionColor`-only 3D path (used as a flat-shading
workaround by the already-ported Primitives3D sample) has no equivalent of XNA's
`VertexPositionNormal` + a normal-lit GLSL shader, so `BasicEffect.EnableDefaultLighting()`
has nothing to render with per-vertex normals.

**Tracked in:** DEFERRED.md item #5.
