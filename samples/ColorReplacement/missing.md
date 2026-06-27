# Missing / Differences from XNA 4.0 original

## Not ported — blocked by Model loading and custom Effect

**XNA behaviour:** Loads a 3D car model (`Car.x`), applies a custom HLSL shader
(`ReplaceColor.fx`) that replaces the car body colour in real time based on
user-adjustable RGB target values.

**CNA port behaviour:** Not implemented. No source files exist yet.

**Root cause:**
- `Car.x` model must be converted to CNA's `.model.json` format (DEFERRED.md item #6).
- `ReplaceColor.fx` HLSL shader must be rewritten as GLSL `.shader.json` (DEFERRED.md item #11).
- CNA supports both Model loading and custom Effects — the gap is asset/shader conversion.

**Tracked in:** DEFERRED.md items #6 (model conversion) and #11 (shader conversion)
