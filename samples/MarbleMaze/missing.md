# Missing / Differences from XNA 4.0 original

**Status: PORTED (2026-07-10).** Full port of `Source/EX2_Polishing/End/` (the
final, polished build — see the "Source tree is a multi-stage tutorial" note
below, kept from the pre-port investigation). Builds with 0 warnings; ran under
`SDL_VIDEODRIVER=x11` for 8+ seconds with no crash across several separate runs.
See the **Verification** section at the bottom for the full account.

Source: `/rv/tmp/XNAGameStudio/Samples/MarbleMaze_4_0/Source/EX2_Polishing/End/MarbleMazeGame/`.

## No `.htm` documentation file exists for this sample
Unlike almost every other XNA 4.0 Phase-6 sample, MarbleMaze ships **no**
`SampleName.htm` anywhere under its source tree (confirmed by a full
`find -iname "*.htm*"` over `MarbleMaze_4_0/`, zero results). Its documentation
instead is a 101-page Word tutorial, `3D Game Development With XNA.doc`. This
directory intentionally has **no** `.htm` file — none was fabricated.
Consequently, the mandatory F1 help overlay's control text couldn't be extracted
via `tools/gen_help_png.py`'s usual `.htm`-table scraper — a one-off script
(`gen_marblemaze_help.py`, not added to `tools/` since it's sample-specific, same
precedent as Graphics3D's/MicrophoneEcho's own one-off `gen_help_png.py` variants)
imports `render_png()` directly and supplies hand-written text describing this
port's actual keyboard control scheme (see the Accelerometer entry below), not a
literal translation of anything in the `.doc`.
**Root cause:** N/A — tooling accommodation only.
**Tracked in:** not planned.

## Source tree is a multi-stage tutorial, not a flat sample
`MarbleMaze_4_0/Source/` contains `EX1_MarbleMazeGame/` (three incremental
tutorial checkpoints — 3D drawing, then movement/camera, then physics) and
`EX2_Polishing/` (`Begin/`/`End/`). This port targets **`EX2_Polishing/End/`**,
the final, fully-polished build — confirmed the right target by reading its
`Objects/DrawableComponent3D.cs`/`Marble.cs`/`Maze.cs`, which are the versions
that actually ship `EnableDefaultLighting()`, audio, and the full
`ScreenManager`-based menu/gameplay flow. `EX1`'s stages were not read beyond
confirming this via directory listing.
**Root cause:** N/A — scoping note only.
**Tracked in:** N/A.

## `Content.Load<Model>` bypassed from the start — DEFERRED.md item #26 assumed, not independently re-confirmed this session
**XNA behaviour:** `DrawableComponent3D.cs` loads every 3D asset via
`Game.Content.Load<Model>(@"Models\" + modelName)` and iterates
`Model.Meshes`/`Model.Meshes[...].Effects` in `Draw()`.
**CNA port behaviour:** Every mesh in this sample (`marble.model.json`'s single
mesh, and `maze1`'s 6 named sub-meshes) is instead loaded via a new
`RawMesh.hpp` (NOXNA) that reads the already-converted `_verts.bin`/`_idx.bin`
sidecars directly and constructs real, normally-initialized
`VertexPositionNormalTexture` objects field-by-field (not a `reinterpret_cast`
on a raw byte blob), uploaded through the typed `VertexBuffer::SetData`
overload — the same bypass shape as `ChaseCamera/src/RawModel.hpp` and
`InverseKinematics/src/CylinderModel.hpp`, generalized here to support several
separately-textured sub-meshes sharing one `BasicEffect` (see the next entry).
**This session did NOT independently re-run the "plain `Content.Load<Model>`
renders nothing" empirical test** that ChaseCamera's own session did before
committing to the bypass — per this task's own brief ("don't wait to see a
blank screen before bypassing... budget for the bypass being needed", NEXT.md
section 8 task 6), the bypass was applied proactively from the start, given
item #26 was already independently confirmed **four** times across
InverseKinematics' and ChaseCamera's own assets (converted through both the
`fbx_ascii2model.py` and `assimp export` + `obj2model.py` pipelines, at vertex
counts from 6 to 32458) before this sample was ported. **Stated explicitly, as
instructed:** this sample's own port does not add a fifth independent
confirmation of item #26 — it assumes the bug based on the existing evidence
record and was not re-tested against a plain `Content.Load<Model>` build.
A second, independent reason would have forced the same `RawMesh` bypass
regardless of item #26's status: `.model.json` has no per-mesh texture field
(item #6's addendum) and this sample's maze genuinely needs 6 differently
textured sub-meshes (see next entry) — `Content.Load<Model>` could not have
produced a correctly *textured* maze either way, so the decision to bypass it
was overdetermined by two separate gaps, not solely item #26.
**Root cause:** see DEFERRED.md item #26 (not re-verified against this
sample's own converted assets this session).
**Tracked in:** DEFERRED.md item #26.

## Maze rendering: `MarbleMazeProcessor`'s custom content-pipeline processor has no CNA equivalent
**XNA behaviour:** `MarbleMazePipeline/MarbleMazeProcessor.cs` is a custom
`ModelProcessor` subclass that (a) chains to the stock processor to build a
normal, renderable `Model` from `maze1.FBX`'s 6 mesh nodes (`walls`, 3
differently-textured `Floor` sub-parts, `topWall`, `floorSides`), and (b)
additionally walks the same node tree at content-**build** time, flattening
each named mesh's absolute-transformed triangle positions into a
`Dictionary<string, List<Vector3>>` stashed on `Model.Tag`, later read back by
`Maze.cs`'s `LoadContent()` for collision.
**CNA port behaviour:** CNA has neither a `Model.Tag` equivalent nor build-time
custom-`ContentProcessor` extensibility (pre-existing DEFERRED.md item #18).
Reconstructed both halves at runtime instead:
- **Rendering:** confirmed via `assimp info maze1.FBX` that the 6 XNA mesh
  nodes map to exactly 6 materials (one texture each, no multi-material
  sub-parts to reconcile) — `assimp export maze1.FBX maze1.obj` preserves each
  as its own named `g`/`usemtl` group. A one-off script
  (`split_maze_obj.py`, not added to `tools/` — sample-specific) splits the
  combined `.obj` into 6 standalone per-group `.obj` files (each keeping the
  *full* shared `v`/`vt`/`vn` pool so face indices stay valid, only filtering
  which `f` lines belong to that group), then each is fed unmodified through
  the existing `tools/obj2model.py` to produce
  `maze_walls`/`maze_floor1`/`maze_bridge`/`maze_floor2`/`maze_topwall`/
  `maze_floorsides` `_verts.bin`/`_idx.bin` pairs. `Maze.hpp` loads all 6 via
  `RawMesh` and draws them sharing one `BasicEffect` (texture swapped per part
  before each draw call) — mirroring what one `BasicEffect` per
  `ModelMeshPart` would have looked like, since `Maze.cs`'s own `Draw()`
  configures every part identically except for its pre-existing `Texture`.
- **Collision:** `RawMesh::ExpandTrianglePositions()` reconstructs exactly what
  `MarbleMazeProcessor.FindVertices()` flattened into `Model.Tag`, from the
  *same* already-loaded vertex/index data (no separate picking/collision
  sidecar file needed, unlike TrianglePicking's `--picking` tool flag): Ground
  = `floor1` + `bridge` + `floor2` triangles concatenated (matching
  `tagData["Floor"]`, which combines all 3 XNA "Floor"-named sub-parts under
  one key); Walls = the `walls` part alone; FloorSides = the `floorSides` part
  alone. `topWall` is rendered but excluded from collision, exactly like the
  original (confirmed by reading `Maze.cs`: only `"Floor"`/`"floorSides"`/
  `"walls"` keys are ever read from `tagData`).
- **Start/Finish/checkpoint bone positions:** `maze1.FBX`'s `Start`/`Finish`/
  `spawnPt1`..`spawnPt4` nodes carry no geometry — pure transform markers,
  matching `Model.Bones[...].Transform.Translation` in the original. Extracted
  once, offline, via a one-off `pyassimp` script that walks the FBX's node
  hierarchy (including the `_$AssimpFbx$_Translation`/`_PreRotation`/
  `_GeometricTranslation` synthetic decomposition nodes assimp introduces) and
  prints each marker node's accumulated world-space transform, then hardcoded
  as `Vector3` constants in `Maze.hpp` (no channel exists in `RawMesh`'s binary
  sidecars for bone-only nodes). Cross-checked for self-consistency against the
  exported mesh geometry's own -450..450 X / -165..395 Z bounding box
  (`assimp info` on `maze1.FBX`) — all 6 marker positions fall within it.
**Root cause:** DEFERRED.md item #18 (no custom `ContentProcessor`
extensibility) plus item #26 (see above).
**Tracked in:** DEFERRED.md item #18, item #26.

## Second confirmed instance of the `assimp export` triangle-winding quirk (not a new bug)
**XNA behaviour:** N/A — real XNA's content pipeline builds correctly-wound
geometry directly from the FBX importer.
**CNA port behaviour:** with CNA's default `RasterizerState::CullCounterClockwise`,
every one of the maze's 6 parts *except* `walls` rendered as **fully invisible**
— confirmed live via screenshot (only `walls` and the marble were visible; the
floor/bridge/topWall/floorSides were simply absent, not culled-looking-black,
just gone). Forcing `RasterizerState::CullNone` around the whole maze draw made
every part appear correctly (fully textured, shaded, and — once the marble
rests on it — visibly casting the maze's own baked ambient-occlusion-style
shadow blobs). Root cause: `assimp export maze1.FBX maze1.obj` re-emits these
5 parts' triangles wound the opposite way from CNA's default winding
convention. This is the exact same `assimp export`-introduced winding
inversion `ChaseCamera`'s `Ground.x` conversion hit (see
`samples/ChaseCamera/missing.md`) — a **second**, independent confirmed
instance of the same tool quirk, this time on an `.FBX` source (not `.x`) and
across 5 differently-shaped mesh parts at once, reinforcing that this is a
property of `assimp export`'s own output convention, not of any one source
format or mesh. Kept as a permanent `RasterizerState::CullNone` around the
whole maze draw call (all 6 parts, not just the affected 5, for simplicity —
`walls` renders identically either way since it doesn't depend on which face
winding is front-facing when neither side is ever hidden by depth in practice).
**Root cause:** `assimp export`'s own triangle-winding convention, opposite to
CNA's `RasterizerState::CullCounterClockwise` default.
**Tracked in:** not a DEFERRED.md item (same class of per-asset accommodation
`HeightmapCollision`'s/`GeneratedGeometry`'s own runtime-built terrain already
needed, and `ChaseCamera`'s `Ground.x` needed before it) — just the second
confirmed sighting, not a new gap.

## Accelerometer input replaced with direct keyboard tilt (established DEFERRED.md item #15 pattern)
**XNA behaviour:** `GameplayScreen.HandleInput()` reads
`Accelerometer.GetState().Acceleration` and converts it into a `maze.Rotation`
delta, clamped to ±30°. `Misc/Accelerometer.cs`'s own
`#if WINDOWS_PHONE`-gated real-sensor path is compiled out on a non-phone
build; its **emulator fallback** (arrow keys synthesizing a fake accelerometer
`Vector3`, `Left`→`X=-.1`, `Right`→`X=.1`, `Up`→`Y=.1`, `Down`→`Y=-.1`) is what
actually runs — and `GameplayScreen`'s own `DeviceType.Emulator` branch (not
`DeviceType.Device`) is what actually executes on any non-phone build.
**CNA port behaviour:** the two-hop indirection (arrow keys → fake
accelerometer vector → rotation delta) is algebraically collapsed into one
direct mapping in `GameplayScreen::HandleInput()`: `Right` → `maze.Rotation.Z
-= angularVelocity`; `Left` → `+= angularVelocity`; `Up` → `maze.Rotation.X -=
angularVelocity`; `Down` → `+= angularVelocity`; both clamped to
±30°/frame-accumulated, exactly matching the original's net effect. This is
the same "un-`#if` the existing non-phone fallback" pattern established for
Yacht/SnowShovel/Bounce (DEFERRED.md item #15) — not an invented control
scheme. `Misc/Accelerometer.cs` itself and the double-tap-to-calibrate branch
(guarded by `DeviceType.Device`, i.e. a real phone only) are not ported at all,
matching this: **no `CalibrationScreen.cs`/calibration UI was ported either** —
it is genuinely dead code on any non-phone build (confirmed by re-reading
`GameplayScreen.cs`'s `CalibrateGame()` call site, which is only reachable
inside the same `DeviceType.Device`-guarded branch).
**Root cause:** N/A — platform-appropriate input substitution, not a CNA gap.
**Tracked in:** DEFERRED.md item #15 (established pattern, not a new item).

## `LoadingAndInstructionScreen`: synchronous asset load instead of a background thread
**XNA behaviour:** `LoadingAndInstructionScreen.cs` spins up a
`System.Threading.Thread` to call `GameplayScreen.LoadAssets()` (which builds
the `Camera`/`Maze`/`Marble` — i.e. creates `VertexBuffer`/`IndexBuffer`/
`Texture2D`/`BasicEffect` GPU objects) while showing "Loading..." text, then
transitions once `thread.ThreadState == ThreadState.Stopped`.
**CNA port behaviour:** `LoadAssets()` is called synchronously from
`Update()` instead — CNA's EasyGL backend's graphics-resource creation is not
confirmed safe to call from a thread other than the one owning the GL context;
introducing a real second thread purely to reproduce this cosmetic loading
animation was judged out of proportion to the risk for a content set this
small (a few small meshes/textures/sounds). One `Update()` call sets a flag and
calls `LoadAssets()` (this frame's `Draw()` still shows "Loading..." since the
transition itself happens the *following* `Update()`), giving at least one
visible frame of the loading text before the switch to `GameplayScreen`, even
though the load itself completes effectively instantly.
**Root cause:** N/A — deliberate simplification given CNA's current
single-GL-thread assumption; not a tracked CNA gap since introducing real
background-thread GPU resource creation wasn't attempted (and isn't known to
be safe or unsafe either way — nobody has tested it).
**Tracked in:** not planned.

## `HighScoreScreen` persistence: `std::fstream` instead of `IsolatedStorageFile`
**XNA behaviour:** `HighScoreScreen.cs`'s `SaveHighscore()`/`LoadHighscore()`
use Windows Phone's `IsolatedStorageFile`/`IsolatedStorageFileStream` to
persist the top-10 table to `highscores.txt` in per-app isolated storage.
**CNA port behaviour:** CNA/sharp-runtime has no `IsolatedStorageFile`
equivalent. Ported with plain `std::ofstream`/`std::ifstream` against
`highscores.txt` in the current working directory — stable and predictable
since every sample in this repo is documented to always be run from its own
binary directory (NEXT.md section 6).
**Root cause:** N/A — no isolated-storage concept exists outside a
Windows/Windows-Phone `System.IO.IsolatedStorage` context.
**Tracked in:** not planned.

## `FinishCurrentGame()`: fixed "Player" name instead of an on-screen keyboard
**XNA behaviour:** when a new high score is achieved,
`GameplayScreen.FinishCurrentGame()` calls
`Guide.BeginShowKeyboardInput(...)` to let the player type their name via the
Windows Phone on-screen keyboard, asynchronously completing via a callback.
**CNA port behaviour:** always records the entry as `"Player"`. CNA does
expose a real `Guide::BeginShowKeyboardInput` (confirmed via header read), but
wiring up its async completion callback for a single cosmetic text-entry field
was judged out of scope for this session given the time already invested in
the sample's core game systems — a reasonable follow-up if ever revisited, not
a CNA gap.
**Root cause:** N/A — scope decision, not a technical blocker.
**Tracked in:** not planned.

## `LinkedList<Vector3>` checkpoints ported as `std::vector<Vector3>` + index
**XNA behaviour:** `Maze.Checkpoints` is a `LinkedList<Vector3>`;
`GameplayScreen` tracks `LinkedListNode<Vector3> lastCheackpointNode` and walks
`.Next` forward each frame in `UpdateLastCheackpoint()`.
**CNA port behaviour:** `Maze::Checkpoints` is a `std::vector<Vector3>` (Start,
then `spawnPt1..spawnPt4` in FBX node order); `GameplayScreen` tracks a
`std::size_t lastCheckpointIndex_` and loops `for (i = lastCheckpointIndex_+1;
i < Checkpoints.size(); ++i)` — the same forward-only linear scan, just over a
vector-by-index instead of a linked list's `.Next` chain. No behavior change.
**Root cause:** N/A — natural C++ adaptation (no reason to use a linked list
for forward-only traversal over a fixed, small checkpoint set).
**Tracked in:** not planned.

## `public new bool IsActive` (C#) ported as a same-named hiding field
**XNA behaviour:** `GameplayScreen`'s `public new bool IsActive = true;` field
hides `GameScreen.IsActive`'s computed transition-state property — a fully
independent "is the game actually being played right now" flag that
`PauseScreen` reads/writes directly (`(ScreenManager.GetScreens()[0] as
GameplayScreen).IsActive = true;`).
**CNA port behaviour:** `GameplayScreen::IsActive` is a plain public `bool`
field with the exact same name as the base `GameScreen::IsActive()` method —
C++ member-hiding rules mean this correctly hides the base method for
unqualified access on a `GameplayScreen&`/`GameplayScreen*` (exactly like C#'s
`new` keyword, just implicit rather than requiring an explicit modifier), so
`gameplayScreen.IsActive` (field, no parens) and `someGameScreen->IsActive()`
(base method call, through a `GameScreen*`) both resolve correctly and
independently, matching the original's dual meaning exactly.
**Root cause:** N/A — direct, faithful language-level translation.
**Tracked in:** not planned.

## Draw order: 3D scene drawn before the 2D `SpriteBatch` block, not interleaved
**XNA behaviour:** `GameplayScreen.Draw()` opens one `SpriteBatch.Begin()`/
`End()` block that wraps *both* the HUD text draws *and* the 3D `maze`/`marble`
draws in the middle, re-enabling `DepthStencilState.DepthBufferEnable` (which
`SpriteBatch.Begin()` disables) right before the interleaved 3D calls.
**CNA port behaviour:** matches this repo's established convention (e.g.
`ChaseCameraGame.hpp`) instead: the 3D scene (`maze_->Draw()`/`marble_->Draw()`)
is drawn first, with no `SpriteBatch` scope open at all, then a single
`SpriteBatch.Begin()`/`End()` block draws all HUD text (start countdown, timer,
game-over message) afterward. This sidesteps needing the
`DepthStencilState`-re-enable dance entirely (depth testing is simply never
disabled in the first place, since no `SpriteBatch.Begin()` runs before the 3D
draws) and produces an identical visual result — the text still draws on top
of the 3D scene either way.
**Root cause:** N/A — reordering for consistency with this repo's other
mixed-3D/2D samples; not a CNA limitation (both orderings are expressible).
**Tracked in:** not planned.

## `DrawableComponent3D::Draw()` is not a generic `Model`-iterating default
**XNA behaviour:** the base `DrawableComponent3D.Draw()` iterates a generic
`Model.Meshes`/`Effects`, shared by every subclass.
**CNA port behaviour:** since every subclass (`Marble`, `Maze`) uses `RawMesh`
instead of a real `Model`, and each needs a different `Draw()` shape (`Marble`:
one mesh, one texture, one owned `BasicEffect`; `Maze`: 6 differently-textured
parts sharing one `BasicEffect`), `DrawableComponent3D` leaves `Draw()` to
`DrawableGameComponent`'s own already-virtual method — each subclass overrides
it directly instead of sharing one generic base implementation. The physics
skeleton (`CalcPhysics`/`CalculateCollisions`/`CalculateAcceleration`/
`CalculateFriction`/`CalculateVelocityAndPosition`/`UpdateFinalWorldTransform`)
is still shared exactly as in the original.
**Root cause:** DEFERRED.md item #26 (forces the `RawMesh` bypass, see above).
**Tracked in:** DEFERRED.md item #26.

## Neither `Marble`/`Maze`/`Camera` is ever added to `Game.Components`
Confirmed by direct reading of `GameplayScreen.cs`: `maze`/`marble`/`camera`
are constructed and have `Initialize()` called directly, but are never passed
to `Components.Add(...)` anywhere — `GameplayScreen` itself calls their
`Update()`/`Draw()` methods directly each frame. DEFERRED.md item #23
(`Game::DoInitialize()`'s `ComponentAdded` timing gap) therefore does not apply
to this sample at all, the same conclusion `HeightmapCollision` reached for
its own terrain/ball. `AudioManager`/`ScreenManager` **are** added to
`Game.Components` (from the `MarbleMazeGame` constructor, i.e. before
`Initialize()` ever runs) — matching the safe, unaffected case item #23's own
text already describes.
**Root cause:** N/A — confirms a non-issue, not a gap.
**Tracked in:** N/A.

## `GraphicsDevice::Clear(Color)` not clearing the depth buffer (DEFERRED.md item #24) — latent, not observed here
`GameplayScreen::Draw()` calls the single-argument `Clear(Color)` overload
every frame before the 3D draws, which (per DEFERRED.md item #24) never clears
the depth buffer. No visible depth-artifact was observed across this session's
screenshots (walls/floor/marble all sort correctly relative to each other,
including the marble appearing correctly in front of / behind maze geometry as
the camera angle changed) — consistent with every other sample's experience of
this same latent gap, not a new finding.
**Root cause:** DEFERRED.md item #24 (unfixed, pre-existing).
**Tracked in:** DEFERRED.md item #24.

## `Viewport::getAspectRatioProperty()` already implemented — DEFERRED.md item #9 is stale
DEFERRED.md item #9 claims `Viewport` has no `AspectRatio` convenience
property. A direct grep of `cna`'s current source
(`include/.../Viewport.hpp:54` and the matching `.cpp` implementation) shows
`getAspectRatioProperty()` already exists and is implemented — used directly
in this sample's `Camera::Initialize()`
(`graphicsDevice.getViewportProperty().getAspectRatioProperty()`), no manual
`(float)Width/Height` workaround needed. Per this repo's own "risky
assumption" caveat (NEXT.md section 5 — DEFERRED.md blockers can go stale since
`cna` is under active concurrent development), this item should be marked
resolved. Updated in DEFERRED.md.
**Root cause:** stale documentation, not a current gap.
**Tracked in:** DEFERRED.md item #9 (marked resolved this session).

## `ScreensGlue.hpp` — a NOXNA organizational file with no C# equivalent
This sample's 6 `GameScreen` subclasses reference each other circularly
(`MainMenuScreen` ↔ `HighScoreScreen`, `PauseScreen` ↔ `MainMenuScreen`,
`PauseScreen` ↔ `GameplayScreen`, `GameplayScreen` ↔ `HighScoreScreen`,
`LoadingAndInstructionScreen` → `GameplayScreen`) — completely unremarkable in
C#, where the whole project compiles as one unit with no header-order
concerns. C++ needs the cycle broken: each screen's own header forward-declares
whichever other screens it needs and only *declares* (doesn't define) any
method whose body needs a not-yet-visible type; `Screens/ScreensGlue.hpp`
includes all 6 headers and defines every deferred method body once every class
is mutually visible — the same technique this repo's own
`ScreenManager.hpp`/`MenuScreen.hpp` already use (in miniature) for
`GameScreen`/`MenuEntry`'s own two-way reference.
**Root cause:** N/A — C++ header-ordering accommodation, not a behavior
difference.
**Tracked in:** N/A.

## Rocklevel1.png / Background.png / SplashScreenImage.jpg not converted
`MarbleMazeGameContent.contentproj` lists `Textures/rocklevel1.png` as a
compiled content item, but no code path in `MarbleMazeGame.cs`/any screen/any
object ever calls `Content.Load<Texture2D>("rocklevel1")` — confirmed by
grepping every `.cs` file in `EX2_Polishing/End/` for the string. Left
unconverted (orphaned asset in the original project too, not a port gap).
`MarbleMazeGame/Background.png` and `MarbleMazeGame/SplashScreenImage.jpg` sit
outside `MarbleMazeGameContent/` entirely (Windows-Phone splash-screen/tile
image project settings, not real runtime `Content.Load` targets) and were
never candidates for conversion.
**Root cause:** N/A.
**Tracked in:** N/A.

## Verification
**Confirmed live, 2026-07-10:**
- Builds `MarbleMaze_cna_samples` with **0 warnings, 0 errors** (verified via a
  from-scratch object-file rebuild, output grepped for "warning"/"error" —
  none found).
- Ran under `SDL_VIDEODRIVER=x11` for 8+ seconds across several separate runs
  with no crash (confirmed via `ps aux` mid-run and clean process exit).
- **Main menu (`BackgroundScreen` + `MainMenuScreen`)** renders correctly:
  `titleScreen.png` background, "Play"/"High Score"/"Exit" entries in the
  correct yellow/white pulsing style, screenshot-confirmed stable over 7+
  seconds.
- **F1 help overlay** renders correctly — confirmed via this repo's
  established temporary debug-auto-trigger pattern (`helpTimer_` forced to
  10.0f, screenshotted, reverted before commit): semi-transparent white panel,
  correct hand-written control text, centered on screen.
- **`GameplayScreen`'s full 3D scene** (`Maze` — all 6 parts — plus `Marble`,
  correctly lit and textured) — confirmed via the same established temporary
  debug-auto-trigger pattern (`MarbleMazeGame::LoadContent()` temporarily
  constructed a `GameplayScreen` directly, calling `LoadAssets()` and
  `AddScreen()`ing it immediately, bypassing the menu/loading flow entirely;
  reverted before commit): the maze floor/walls/bridge/topWall/floorSides all
  render fully textured and shaded (after the `RasterizerState::CullNone` fix
  above — before it, only `walls` was visible); the marble renders textured
  and lit, resting stably on the floor at its start position with **no
  falling-through or jitter** over 11+ seconds of passive observation (no
  input) — direct confirmation the ground collision detection
  (`Maze::GetCollisionDetails`/`TriangleSphereCollisionDetection`) works
  correctly end-to-end against the runtime-reconstructed triangle lists.
- **Maze tilt physics** — confirmed via a second temporary debug override
  (`GameplayScreen::HandleInput()` temporarily forced a constant tilt input
  regardless of actual key state, reverted before commit): screenshots over
  several seconds show the maze visibly rotating/tilting, the marble staying
  correctly positioned on the now-tilted floor (not falling off, not clipping
  through walls), and the tilt correctly clamping once it reaches its ±30°
  limit — direct confirmation `Maze::Rotation`'s effect on rendering
  (`Marble`/`Maze`'s shared `Matrix::CreateFromYawPitchRoll(Maze.Rotation...)`
  multiply) and the physics-in-unrotated-frame design (marble's actual X/Z
  position/velocity are computed entirely independent of the tilt, only the
  *display* transform rotates — matching the C# original's own architecture)
  both work correctly.
- **No near-plane-clipping-family symptom (item #26's predecessor theory)
  observed** — this sample's camera sits at a fixed
  `(0,450,100)`/target-offset `(0,0,-50)` relative to the marble, i.e. well
  under the ~1000+ unit distances where that symptom was historically
  observed, and (since every mesh here bypasses `Content.Load<Model>` via
  `RawMesh`) item #26 itself never had a chance to apply either. Consistent
  with, not new evidence for or against, the existing item #26 hypothesis.
- `xdotool getactivewindow` showed a different real user window had focus
  throughout every attempt this session (confirmed multiple times), and a
  synthetic `xdotool key --window <id> Right` sent to the sample's own window
  ID produced no observable effect on a follow-up screenshot — consistent with
  this repo's known shared-desktop `xdotool` reliability caveat, not a code
  bug. **Not independently live-tested via real input:** clicking through
  "Play" → `LoadingAndInstructionScreen`'s tap-to-load → the real (non-debug)
  transition into `GameplayScreen`; `PauseScreen`'s Escape-key trigger and its
  three menu entries; `HighScoreScreen`'s display/tap-to-return; the
  reach-the-`Finish`-marker win condition; falling into a pit and returning to
  the last checkpoint. All of these are the *same* code paths exercised by the
  debug-auto-trigger tests above (`GameplayScreen`'s own `Update()`/`Draw()`/
  physics, `ScreenManager`'s `AddScreen`/`RemoveScreen`/transition machinery
  already proven correct by the menu↔F1-overlay tests) — not entirely
  unverified, but the specific input-triggered *transitions between* screens
  were not clicked through live this session.
