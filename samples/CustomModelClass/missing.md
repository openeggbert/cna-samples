# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-06.** Builds with 0 warnings. Ported using CNA's stock
`Model`/`BasicEffect` (see the item #5 write-up below) instead of replicating the
C# original's own `CustomModel`/`ModelPart` class, since CNA has no build-time
custom-`ContentProcessor` extensibility to reproduce `CustomModelProcessor`
faithfully (item #18) — the runtime `CustomModel` class only ever draws stock
`BasicEffect` geometry anyway, so a stock `Model` produces the same on-screen
result. Copied the already-converted, already-proven `tank.model.json` (+ its 12
per-mesh `.bin` files) directly from `samples/CameraShake/Content/` — no new asset
conversion needed, byte-identical asset.

## CNA gap (pre-existing, not introduced by this port): near-plane clipping renders the tank as a thin line, not the model
**Confirmed live:** running this port shows a thin diagonal white line/dashes on
the CornflowerBlue background instead of a recognizable tank. Before assuming this
was a bug in the port, built and ran the already-shipped `CameraShake` sample (which
renders the *same* `tank.model.json` asset) side by side — it shows the **exact
same** thin-line artifact in the exact same screen position. This confirms the
render defect is `cna`'s own EasyGL-backend rendering of this model/vertex format
family, not anything introduced by this port. This is the same, already-tracked gap
described in this repo's `NEXT.md` ("CameraShake has a near-plane clipping bug in
the EasyGL backend... not yet root-caused") — not a new one. Not attempted to fix
here, per this repo's convention of tracking framework-level rendering bugs
centrally rather than re-diagnosing them per affected sample; see `NEXT.md` section
8 ("Investigate CameraShake's near-plane clipping bug in the EasyGL backend") for
the existing task. This sample should be re-screenshotted once that bug is fixed to
confirm the tank actually renders.

Source: `/rv/tmp/XNAGameStudio/Samples/CustomModelClassSample_4_0/
{CustomModelSample/{CustomModel.cs, CustomModelSampleGame.cs}, CustomModelPipeline/
{CustomModelContent.cs, CustomModelProcessor.cs}}`.

## Blocker (historical, resolved): BasicEffect per-vertex lighting (`EnableDefaultLighting`)
**XNA behaviour:** `CustomModel.cs:81` calls `effect.EnableDefaultLighting()` on the
`BasicEffect` used by every model part before drawing it (`CustomModel.Draw()`,
lines 72-106). The sample's whole point is demonstrating a simplified custom
replacement for XNA's `Model` class, but it still renders through a real,
lit `BasicEffect` — same class of gap as every other item #5 sample.

**CNA port behaviour:** Ported — `LoadContent()` iterates `model_->getMeshesProperty()`
/ `mesh->getEffectsPropertyMutable()` and calls `EnableDefaultLighting()` on each
`BasicEffect` once (a persistent effect property, not per-frame state, so calling it
once at load time is equivalent to the original's per-`Draw()`-call placement).

**Root cause (historical):** was a missing lit-shader path for `VertexPositionNormalTexture`
in CNA; now resolved (see Status note above and DEFERRED.md item #5).

**Note on why this is NOT also a bone-hierarchy (item #6) blocker, despite sharing
`tank.fbx` with SimpleAnimation/SplitScreen:** confirmed by grep — unlike `Tank.cs`
(used by SimpleAnimation/SplitScreen/ChaseCamera's sibling samples), this sample's
own `CustomModel.cs`/`CustomModelSampleGame.cs` contain **zero** `Bones[...]`
lookups. That's because this sample ships its **own** content-pipeline processor
(`CustomModelPipeline/CustomModelProcessor.cs`), which explicitly bakes every node's
transform into its geometry at content-build time and then discards the hierarchy
entirely (`ProcessNode()`: "Meshes can contain internal hierarchy ... but this sample
isn't going to bother storing any of that data" — `node.Transform = Matrix.Identity`
right after `MeshHelper.TransformScene`). The runtime `CustomModel` class is just a
flat `List<ModelPart>`, each drawn with the *same* shared `world` matrix
(`CustomModelSampleGame.cs`: `world = Matrix.CreateRotationY(time * 0.1f)`; the tank
rotates as one rigid whole, wheels/turret do not move independently in this sample).
So the model's per-mesh bone gap that blocks SimpleAnimation/SplitScreen simply
doesn't apply here — DEFERRED.md item #6's summary table also lists this sample
under the bone-hierarchy blocked set, but that appears to be inherited from "uses the
shared `tank.fbx` asset" rather than a per-sample source audit; item #5's own section
(which does list this sample by name against the actual `EnableDefaultLighting()`
call site) is the accurate one for this specific sample, confirmed directly against
the C# source above.

**Tracked in:** DEFERRED.md item #5 (resolved).

## No SpriteBatch/text in the original; F1 help overlay is a pure CNA addition
**XNA behaviour:** N/A — the original has no `SpriteBatch`, no `SpriteFont`, no
on-screen text at all; it just draws the rotating tank.
**CNA port behaviour:** Added a `SpriteBatch` solely for the mandatory F1 help
overlay (per CLAUDE.md) — no other UI. No font asset needed since the overlay is a
pre-rendered PNG, not drawn text.
**Root cause:** N/A — CNA-only addition, not a difference from the original's own
behavior.
**Tracked in:** not planned (by design, see CLAUDE.md's F1 Help Overlay section).

## Verification
**Confirmed live:** built cleanly (0 warnings). Ran under `SDL_VIDEODRIVER=x11` —
process stays up with no crash. Screenshot shows only the pre-existing near-plane
clipping artifact described above (confirmed identical to CameraShake's own, not a
new regression) — the tank itself isn't visibly recognizable in the current
screenshot as a result of that unrelated, already-tracked framework bug, not this
port's own code. F1 help overlay and Escape/gamepad-Back exit were not separately
exercised via synthetic input this session (same `xdotool` reliability caveat as
other samples in this repo) but are straightforward, unconditional code paths with
no dependency on the broken rendering above.
