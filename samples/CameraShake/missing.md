# Missing / Differences from XNA 4.0 original

## Not ported — blocked by Model loading

**XNA behaviour:** Loads a 3D tank model (`tank.fbx`) and ground plane (`Ground.x`).
A camera-shake effect is applied when the user taps (or triggers a shake event),
creating an oscillating view offset to simulate explosion impact.

**CNA port behaviour:** Not implemented. No source files exist yet.

**Root cause:** `Content.Load<Model>()` is not implemented in CNA (DEFERRED.md item #6).

**Tracked in:** DEFERRED.md item #6 (Model loading)
