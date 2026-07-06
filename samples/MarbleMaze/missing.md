# Missing / Differences from XNA 4.0 original

**Status: UNBLOCKED, not yet ported — corrected 2026-07-06.** The blocker below was
accurate when first written this session, but a live build+run of `cna_test_
easygl_basiceffect_combinations` right after found CNA's `VertexPositionNormalTexture`
lit path (exactly what this sample's `Content.Load<Model>` calls produce) already
works — case "(e) Directional lighting" passes (exit code 0). DEFERRED.md item #5
is marked resolved for `Model`-based samples. No CNA gap remains; this is now a
normal, straightforward porting candidate (target `Source/EX2_Polishing/End/` as
noted below). No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's "Adding a new
sample" steps. (Kept the original write-up below.)

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

**CNA port behaviour:** Resolved 2026-07-06 — see Status note at the top of this
file. CNA's `VertexPositionNormalTexture` lit path is real and tested.

**Root cause (historical):** was a missing lit-shader path for `VertexPositionNormalTexture`
in CNA; now resolved.

**Tracked in:** DEFERRED.md item #5 (resolved).
