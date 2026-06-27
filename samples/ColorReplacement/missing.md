# Missing / Differences from XNA 4.0 original

## Not ported — blocked by Model loading and custom Effect

**XNA behaviour:** Loads a 3D car model (`Car.x`), applies a custom HLSL shader
(`ReplaceColor.fx`) that replaces the car body colour in real time based on
user-adjustable RGB target values.

**CNA port behaviour:** Not implemented. No source files exist yet.

**Root cause:**
- `Content.Load<Model>()` is not implemented in CNA (DEFERRED.md item #6).
- Custom HLSL Effect loading (`Content.Load<Effect>()`) is not implemented in CNA.

**Tracked in:** DEFERRED.md items #5 (custom shaders) and #6 (Model loading)
