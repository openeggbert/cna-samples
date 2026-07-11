z# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-09.** Builds with 0 warnings. Ported using CNA's stock
`Model`/`BasicEffect`, `DrawableGameComponent`, and `OcclusionQuery` — no CNA-side
API gaps for this sample's own code (the historical blocker this sample was
originally written against, below, is resolved for `Model`-based rendering).
Confirmed live for 5+ seconds with no crash; F1 help overlay confirmed via a
temporary debug auto-trigger (removed before commit, same established pattern as
other samples — `xdotool` reached the window's focus but not its keyboard state on
this shared desktop, consistent with this repo's known `xdotool` reliability
caveat).

Source: `/rv/tmp/XNAGameStudio/Samples/LensFlareSample_4_0/LensFlare/
{Game.cs, LensFlareComponent.cs}`.

## Asset conversion bug found and fixed: `tools/fbx_ascii2model.py` ignored the FBX node's baked axis-correction transform
**XNA behaviour:** the content pipeline's FBX importer applies each `Model` node's
`PreRotation`/`LclRotation`/`LclScaling`/`LclTranslation` properties when baking
world-space vertex data, so `terrain.fbx` (whose `Plane01` mesh has a baked
`PreRotation` of `-90,0,0` — the standard 3ds Max Z-up → FBX-declared-Y-up
correction) renders as a flat, gently bumped ground plane extending in X/Z with
height in Y.

**CNA port behaviour (before the fix):** `tools/fbx_ascii2model.py` baked the raw
`Vertices:`/`Normals:` data straight into the output buffers with no node-transform
applied at all. The result had all height variation in Z (not Y) and a full X/Y
footprint of roughly ±250 units — i.e. the "terrain" stood on its edge, entirely
outside the camera's near/far view volume. Screenshot before the fix: solid
CornflowerBlue, nothing visible at all (not even the near-plane-clipping thin-line
artifact described below).

**Root cause:** the converter never parsed or applied `Model` node transform
properties, only raw geometry data.

**Fix:** `tools/fbx_ascii2model.py` now parses each mesh's `PreRotation`/
`PostRotation`/`Lcl Rotation`/`Lcl Scaling`/`Lcl Translation` properties and applies
the composed rotation/scale/translation to every position and normal before
building the vertex buffers (identity transform is skipped as a no-op). Regenerated
`samples/LensFlare/Content/terrain.model.json` + its two `.bin` buffers with the
fixed converter — after the fix, the terrain's Y range is `0..63.5` (height, as
expected) and X/Z span the ±250-ish ground footprint, matching the original's
scale. **Confirmed this does not change any other already-shipped asset:**
`tank.fbx` (`CameraShake`/`CustomModelClass`'s source) has `PreRotation 0,0,0` on
every mesh, so the converter's output for it is unaffected by this fix — verified
by regenerating it into a scratch directory and diffing against the checked-in
`tank.model.json`/`.bin` files (identical). See DEFERRED.md item #6's addendum for
the full write-up, including a note that `tank.fbx`'s *sub-meshes* (wheels/turret/
etc.) do have non-zero `Lcl Translation` that this fix would now bake in if
`tank.model.json` were ever regenerated — intentionally left alone here since it's
outside this sample's scope and doesn't affect `CameraShake`/`CustomModelClass`
today (neither moves tank parts independently).

**Tracked in:** DEFERRED.md item #6 (addendum, tool fix already applied — this is
not a CNA gap, so there is nothing further to resolve there).

## CNA gap (pre-existing, not introduced by this port): near-plane clipping renders the terrain as a thin line, not a ground plane
**Confirmed live**, after the asset-conversion fix above: the terrain (now correctly
oriented and scaled) still renders as a thin diagonal white line/dashes on the
CornflowerBlue sky background, in the same style as `CameraShake`'s and
`CustomModelClass`'s tank rendering. This confirms `NEXT.md` section 4's "shared
framework cause" hypothesis for the EasyGL near-plane-clipping bug extends beyond
`tank.model.json` to a second, independently-converted FBX asset with completely
different geometry (a mostly-flat multi-triangle grid vs. a multi-part rigid
model) — ruling out an asset-specific cause a second time. Not attempted to fix
here, per this repo's convention of tracking framework rendering bugs centrally;
see `NEXT.md` section 8 task 2 ("Investigate the EasyGL near-plane clipping bug
itself").
**Tracked in:** `NEXT.md` section 4/8 (task 2), pre-existing.

## New CNA gap found: EasyGL backend never applies `BlendState.ColorWriteChannels` (no `glColorMask`)
**XNA behaviour:** `LensFlareComponent.UpdateOcclusion()` draws a small
`querySize`×`querySize` quad at the sun's screen position, using a custom
`BlendState` with `ColorWriteChannels = ColorWriteChannels.None` specifically so
this quad is invisible on screen — only its occlusion-query pixel count matters,
never its actual color output.
**CNA port behaviour:** the quad renders as a fully visible, opaque white square
(confirmed live via screenshot) sitting on top of the scene at the sun's screen
position. Confirmed via direct source grep — `grep -rn "ColorWriteChannels\|
glColorMask" src/CNA/Internal/Backends/EasyGL/` in `cna` returns zero matches — the
EasyGL backend parses/stores `BlendState.ColorWriteChannels` but never calls
`glColorMask` (or equivalent) to actually apply it to GL state, so every draw call
writes all four color channels regardless of the active `BlendState`.
**Root cause:** `EasyGLGraphicsBackend` has no color-write-mask application in its
state-application path.
**Tracked in:** DEFERRED.md item #22 (new, not started).

## Occlusion-driven glow/flare sprites not observed to appear during this verification session
**Observation:** over 5+ seconds of live running (default camera/light position,
no user input), the glow and flare sprites (`LensFlareComponent::DrawGlow()`/
`DrawFlares()`, gated on `occlusionAlpha_ > 0`) never appeared — only the always-
visible white occlusion-query quad (see above) and the near-plane-clipped terrain
line were visible.
**Analysis:** `OcclusionQuery` itself is independently documented as fully correct
on the EasyGL backend (`cna/docs/occlusionquery-support.md`), so the query
mechanism is not itself suspected. Whether this is a downstream symptom of the
`ColorWriteChannels` gap above (e.g. some other piece of state also not being
restored/applied correctly around the query draw), a separate small bug, or simply
expected for this exact camera/light configuration (the terrain's near-plane-
clipping artifact means most of the "sky" is never depth-occluded, which should, if
anything, make the sun's occlusion query pass and the flares appear — the opposite
of what was observed) was not root-caused in this session; doing so would go beyond
this task's scope (port the sample, confirm the near-plane-clipping bug against a
second asset). Flagged here rather than silently assumed to be correct.
**Tracked in:** not yet filed as its own DEFERRED.md item — needs isolated
investigation before that's warranted (could be entirely explained by item #22
above once that's fixed; re-check then).

## No SpriteBatch/text in the original; F1 help overlay is a pure CNA addition
**XNA behaviour:** N/A — the original has no `SpriteBatch`, no `SpriteFont`, no
on-screen text at all; the game class itself never creates a `SpriteBatch` (only
`LensFlareComponent` does, for its own glow/flare sprites).
**CNA port behaviour:** added a second, separate `SpriteBatch` in `LensFlareGame`
solely for the mandatory F1 help overlay (per `CLAUDE.md`) — no other UI. Verified
via a temporary debug auto-trigger (`helpTimer_ = 10.0f` forced in `LoadContent()`,
removed before commit): the overlay renders centered, on top of everything, exactly
as designed.
**Root cause:** N/A — CNA-only addition, not a difference from the original's own
behavior.
**Tracked in:** not planned (by design, see `CLAUDE.md`'s F1 Help Overlay section).

## `ground.png` texture unused (same limitation as every other `.model.json`-based sample)
**XNA behaviour:** the terrain's diffuse texture (`ground.png`) is assigned via the
content pipeline's material/texture binding on the FBX mesh at build time; `Game.cs`
itself never explicitly sets `effect.Texture`.
**CNA port behaviour:** `ground.png` was copied into `Content/` for completeness but
is never actually bound to the terrain's `BasicEffect` — CNA's `.model.json`
"meshes" schema (`ModelTypeReader::Read()` in `ContentManager.cpp`) has no
`"texture"` field for this mesh format (confirmed by direct source read), so the
terrain renders as a flat lit/fogged surface with no diffuse texture. This is the
same, already-accepted limitation `CustomModelClass`'s `tank.model.json` has (no
port in this repo currently has a textured `.model.json` mesh).
**Root cause:** `ModelTypeReader::Read()`'s simple mesh schema doesn't parse a
texture reference.
**Tracked in:** not filed as a new DEFERRED.md item — same class of gap already
implicitly covered by item #6 (model asset conversion); worth a small addendum
there if a future sample specifically needs a textured static model.

## Verification
**Confirmed live:** built cleanly (0 warnings). Ran under `SDL_VIDEODRIVER=x11` for
5+ seconds — process stays up with no crash, no error output beyond normal EasyGL
backend init logging. Screenshot shows the pre-existing near-plane-clipping
artifact (confirmed identical in kind to `CameraShake`'s/`CustomModelClass`'s own,
on a second, independent asset) plus the new `ColorWriteChannels` artifact
described above — neither is a regression introduced by this port. F1 help overlay
confirmed via a temporary debug auto-trigger (removed before commit). Escape-to-
exit and gamepad-Back-to-exit are straightforward, unconditional code paths
ported verbatim from the original and were not separately exercised via synthetic
input this session (`xdotool` reached the window's focus but not its keyboard
state — same reliability caveat noted elsewhere in this repo).
