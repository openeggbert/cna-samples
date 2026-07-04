# Missing / Differences from XNA 4.0 original

## RemoteDebugCommand omitted
**XNA behaviour:** `GameDebugTools/RemoteDebugCommand.cs` lets a Windows PC drive the
in-game debug console on an Xbox 360 over a SystemLink connection, via
`Microsoft.Xna.Framework.Net.NetworkSession` and `Microsoft.Xna.Framework.GamerServices`.
**CNA port behaviour:** Not ported at all.
**Root cause:** Depends on Xbox 360 SystemLink networking and Xbox Live GamerServices,
neither of which exist in CNA (or make sense on desktop). The original file itself
already excludes this class on Windows Phone (`#if !WINDOWS_PHONE`) for the same class
of reason.
**Tracked in:** not planned.

## Flat shading instead of lit shading (spheres)
**XNA behaviour:** `GeometricPrimitive`/`SpherePrimitive` use `VertexPositionNormal` +
`BasicEffect.EnableDefaultLighting()`, giving the bouncing spheres a lit, shaded 3D
appearance.
**CNA port behaviour:** `VertexPositionNormal` is not supported by CNA. The port uses
`VertexPositionColor` with white vertex colors and an unlit `BasicEffect`
(`DiffuseColor` supplies the flat per-sphere tint instead). All spheres render as flat
single-color circles rather than shaded 3D balls.
**Root cause:** Same simplification already established in `samples/Primitives3D`
(DEFERRED.md item 5) — this port's `Primitives/GeometricPrimitive.hpp`/
`SpherePrimitive.hpp` are a fresh copy of that sample's files (per the "no shared
sample library" rule), not shared code.
**Tracked in:** DEFERRED.md item 5.

## Ground model converted from .x to .model.json; texture not applied
**XNA behaviour:** Loads `Ground.x` (a 2-triangle, 4-vertex flat quad) as a `Model` and
draws it scaled to the world size, textured with a repeating `Checker.bmp` via
`SamplerState.LinearWrap`.
**CNA port behaviour:** `Ground.x` was converted to CNA's `.model.json` + binary
vertex/index format using `assimp export Ground.x ground.obj` followed by
`tools/obj2model.py` (the same two-step FBX/X → OBJ → `.model.json` pipeline already
established in `samples/CameraShake`). The ground renders as an untextured
`BasicEffect` plane — `Checker.png` is present in `Content/` for reference but is not
wired up, since `.model.json`'s `"effect": "BasicEffect"` field has no way to assign a
texture (same limitation already documented in CameraShake's missing.md).
**Root cause:** No content-pipeline texture-to-mesh assignment in `.model.json`.
**Tracked in:** DEFERRED.md (model/texture pipeline gap) — same entry as CameraShake.

## TimeRuler/DebugCommandUI profiling always active; no threading guards
**XNA behaviour:** Every `TimeRuler` profiling method (`StartFrame`/`BeginMark`/
`EndMark`/`ResetLog`/the `Draw(Vector2,int)` overload) is wrapped in
`[Conditional("TRACE")]` so a release build compiles them away entirely, and the
shared marker state is protected with `lock(this)` plus `Interlocked` operations on
the update-count guard (defensive against XNA's fixed-timestep `Update` catch-up loop,
which can call `Update` — and therefore `StartFrame` — more than once before the next
`Draw`).
**CNA port behaviour:** The profiling code is unconditionally compiled in (there is no
TRACE/release build distinction in this repo), and the locking is dropped in favor of
a plain `int` counter — this is a single-threaded desktop game loop, so there is
nothing to guard against. The `StartFrame` re-entrancy guard itself (skip resetting
the frame if `Update` runs more than once before the next `Draw`) is preserved.
**Root cause:** No release/debug split or multithreaded Update/Draw in this project.
**Tracked in:** not planned.

## StringBuilderExtensions not ported
**XNA behaviour:** `GameDebugTools/StringBuilderExtensions.cs` hand-writes a
no-allocation `int`/`float`-to-`StringBuilder` digit formatter, to avoid GC pressure
from `StringBuilder.AppendFormat`/boxing on Xbox 360/Windows Phone.
**CNA port behaviour:** Plain `std::string`/`snprintf`-based formatting is used
wherever the original called `AppendNumber` (FPS display, TimeRuler's log string).
**Root cause:** The original's entire purpose was avoiding .NET GC allocations; C++
`std::string`/stack buffers don't have that concern, so porting the manual
digit-extraction algorithm would add complexity with no benefit here.
**Tracked in:** not planned.

## Font substituted
**XNA behaviour:** `Font.spritefont` requests "Segoe UI Mono", 14pt, regular, ASCII
32-126.
**CNA port behaviour:** Generated from `DejaVuSansMono.ttf` at the same size/range via
`tools/make_font.py`, per CLAUDE.md's asset conversion policy.
**Root cause:** No SpriteFont `.xnb` pipeline / Segoe UI Mono unavailable on Linux.
**Tracked in:** CLAUDE.md "Assets" — established pattern, same as every other sample.

## F1 help overlay text authored, not extracted from the .htm
**XNA behaviour:** N/A — F1 help is a CNA-only addition (see CLAUDE.md).
**CNA port behaviour:** `PerformanceMeasuring.htm` (copied verbatim from the original
kit) has no "Sample Controls" table for `tools/gen_help_png.py` to extract — unlike
most other samples' `.htm` files. The control text shown by F1 was authored manually
(reusing `gen_help_png.py`'s own `render_png`/`build_text` helpers so the visual style
matches every other sample's help overlay exactly) from the game's own in-scene
`instructions` string plus the GameDebugTools' own `Tab` control.
**Root cause:** This sample's original documentation simply never included a controls
table (all the real control info lives in the in-game `instructions` string instead).
**Tracked in:** CLAUDE.md "Sample documentation" — same class of deviation as
CatapultWars' authored `.htm` (there the whole doc was missing; here only the table).

## Viewport not queried during Initialize()/LoadContent()
**XNA behaviour:** N/A internally — XNA's `GraphicsDevice.Viewport` is correct from
the moment the device exists.
**CNA port behaviour:** `TimeRuler::LoadContent()` (which runs synchronously inside
`Game::Initialize()`, since `DrawableGameComponent::Initialize()` calls `LoadContent()`
once) uses `GraphicsDeviceManager::DefaultBackBufferWidth/Height` constants instead of
querying `getViewportProperty()`, to avoid the known stale-viewport-during-Initialize
gotcha (NEXT.md section 5; already hit and worked around in SnowShovel/UISample).
**Root cause:** Same underlying CNA viewport-timing issue documented for those two
samples; not re-diagnosed here.
**Tracked in:** NEXT.md section 5 (existing "needs verification" entry).

## Minor renames (no behavior change)
- `IDebugEchoListner` (a typo in the original) → `IDebugEchoListener`.
- The `Color` field on `Marker`/`MarkerLog` (TimeRuler) and `Sphere` → `BarColor`/
  `SphereColor` respectively — C++ doesn't allow a member to share its declaring type's
  name without shadowing that type name for the rest of the class (`Color Color;`
  makes `Color` as a *type* unusable afterward in that scope); this is a mechanical
  fix, not a behavior change.

## Interactive verification
Confirmed by screenshot this session: FPS counter, TimeRuler bars (`Bar 0 Update`/
`Bar 0 Draw`) advancing with live avg times, 200-sphere pool bouncing/colliding in the
world (flat-shaded, per above), the demo text panel (sphere count / collisions state /
instructions), `X` toggling `Collisions Enabled` in that panel, `Tab` opening the debug
command console, typing text plus `Backspace` editing the command line correctly,
`help` executing and echoing all five registered commands (`tr`, `fps`, `cls`, `echo`,
`help` — confirming `TimeRuler`/`FpsCounter` correctly self-register via
`IDebugCommandHost`), and the F1 help overlay.

`Tab` closing the console again, and `Up`/`Down` incrementing/decrementing the sphere
count, were **not** independently re-confirmed — interactive testing was cut short
when a screenshot showed unexpected text in the console's command line that this
session never typed, strongly suggesting a moment of real keyboard input from the
desktop's actual user crossed into this test window (the reverse direction of the
shared-desktop focus flakiness already documented in NEXT.md section 5). Both
untested paths are simple, symmetric with already-confirmed logic (`Tab` closing
mirrors the already-confirmed `Tab` opening; `Up`/`Down` mirror `X`'s already-confirmed
rising-edge-free polling loop), so they're expected to work, but this is worth a
follow-up screenshot check once the desktop is free (see NEXT.md section 8).
