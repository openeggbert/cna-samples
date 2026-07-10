# Missing / Differences from XNA 4.0 original

## Not ported — blocked by custom Effect

**XNA behaviour:** Loads a 3D car model (`Car.x`); most mesh parts use a plain
`BasicEffect`, but the car body part specifically uses a custom HLSL shader
(`ReplaceColor.fx`) that replaces the car body colour in real time based on
user-adjustable RGB target values (`Game.cs`'s `DrawModel()` sets a
`TargetColor` effect parameter on that one part every frame).

**CNA port behaviour:** Not implemented. No source files exist yet.

**Root cause:**
- `ReplaceColor.fx` HLSL shader must be rewritten as GLSL `.shader.json`
  (DEFERRED.md item #11) — no tooling exists yet for this conversion.
- `Car.x` model conversion itself is **not** a blocker — DEFERRED.md item #6
  covers static/rigid-bone model asset conversion, which is fully resolved
  (proven across 6+ shipped samples via the `RawModel.hpp`/`RawMesh.hpp`
  bypass pattern, e.g. `samples/ChaseCamera`, `samples/MarbleMaze`).
  Corrected here 2026-07-10 after this entry was found stale during a
  live re-check — do not cite item #6 as a blocker for this sample again.
- There is no meaningful reduced-scope port: the colour-replacement effect
  *is* the entire point of this sample (its only interactive mechanic), so
  substituting `BasicEffect` for the body part would leave nothing to
  demonstrate.

**Tracked in:** DEFERRED.md item #11 (shader conversion) only.
