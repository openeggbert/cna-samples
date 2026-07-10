# Missing / Differences from XNA 4.0 original

**Status: ported (2026-07-10).** Full C++ port under `src/`, wired into the root
`CMakeLists.txt`. Builds 0 warnings; screenshot-verified rendering, ball physics,
and the invented keyboard-tilt substitute (see Verification below).

Source: `/rv/tmp/XNAGameStudio/Samples/TiltPerspective_4_0/TiltPerspective/
{TiltPerspectiveSample.cs, AccelerometerHelper.cs, BallSimulation.cs, DebugDraw.cs,
RandomUtil.cs, GeometricPrimitive.cs, SpherePrimitive.cs, VertexPositionNormal.cs}`.

This is the second of the two samples covered by the 2026-07-10 user go/no-go
decision (see NEXT.md section 8 task 9) -- with this sample shipped, both halves
of that decision are complete (AccelerometerSample was ported in the prior
session). See NEXT.md's closing note: no further approved/queued porting work
remains as of this session.

## Keyboard-tilt control scheme -- genuinely invented (NOXNA), unlike AccelerometerSample's

**XNA behaviour:** `AccelerometerHelper.cs` wraps a real
`Microsoft.Devices.Sensors.Accelerometer` **unconditionally** -- unlike
`Accelerometer.cs` in AccelerometerSample, this file has **no** `#if
WINDOWS_PHONE` split at all (confirmed by direct read of the whole file, and by
`TiltPerspective.csproj`, which only defines a `Windows Phone` build
configuration). At `Initialize()` time it only tries to `Start()` the real
sensor when `Microsoft.Devices.Environment.DeviceType ==
Microsoft.Devices.DeviceType.Device`; otherwise (including "running in the
emulator," the only reachable case on a desktop port) it sets `Sensor = null`
and `Enabled = true`, and every `Update()` synthesizes a **non-interactive,
purely time-driven wobble**:

```csharp
FakeRollTheta += (float)gameTime.ElapsedGameTime.TotalSeconds * FakeRollSpeed;
FakeRollTheta = MathHelper.WrapAngle(FakeRollTheta);
RawAcceleration = new Vector3(
    (float)Math.Sin(FakeRollPhi) * (float)Math.Cos(FakeRollTheta),
    (float)Math.Sin(FakeRollPhi) * (float)Math.Sin(FakeRollTheta),
    -(float)Math.Cos(FakeRollPhi));
SmoothAcceleration = RawAcceleration;
```

There is no `Keyboard`/`GamePad` reference anywhere in `AccelerometerHelper.cs`
-- confirmed by a full-file read and a grep for both symbols. This is a
materially different situation from AccelerometerSample's own
`Accelerometer.cs` (and Yacht's/SnowShovel's/Bounce's own accelerometer
fallbacks), all of which already ship a genuine keyboard-emulation branch
nested inside their own `#if WINDOWS_PHONE` block (DEFERRED.md item #15's
2026-07-10 correction) that this repo's convention is to "un-`#if`" rather than
invent from scratch. This sample's own task brief explicitly called for
checking that correction before assuming an invented scheme was still needed
here -- it was checked (this section is the result), and the premise holds:
**there is no existing keyboard branch to promote here.** A scheme genuinely
had to be designed from scratch, per the 2026-07-10 user go/no-go.

**CNA port behaviour:** `src/AccelerometerHelper.hpp`'s `Update()` replaces the
sinusoidal wobble above with:

```cpp
Vector3 stateValue;
stateValue.Z = -1.0f;
if (keyboardState.IsKeyDown(Keys::Left))  stateValue.X -= 1.0f;
if (keyboardState.IsKeyDown(Keys::Right)) stateValue.X += 1.0f;
if (keyboardState.IsKeyDown(Keys::Up))    stateValue.Y += 1.0f;
if (keyboardState.IsKeyDown(Keys::Down))  stateValue.Y -= 1.0f;
stateValue.Normalize();
rawAcceleration_ = stateValue;
smoothAcceleration_ = stateValue; // no lerp, matching the original's own Sensor==null path
```

**Why this exact mapping:** rather than inventing an unrelated scheme, this
reuses the same X/Y-arrow-key/Z=-1/`Normalize()` shape this repo already ported
*literally* from AccelerometerSample's and Yacht's own C# **emulator**
fallback branches (`samples/AccelerometerSample/src/Accelerometer.hpp`,
`samples/Yacht/src/Accelerometer.hpp`) -- consistent with the task brief's own
suggestion ("for parity with AccelerometerSample's own approach"). `Z` is held
at `-1` (matching this file's own initial/rest value,
`RawAcceleration = new Vector3(0, 0, -1)` -- "gravity straight down through the
back of the device" when level), and `Left`/`Right` drive `X` while `Up`/`Down`
drive `Y`, exactly like the two precedents above. This keeps every downstream
consumer of `RawAcceleration`/`SmoothAcceleration` (`BallSimulation.hpp`,
`TiltPerspectiveGame.hpp`'s `ComputeEyeVector()`/lighting) completely
unmodified -- only the *source* of these two vectors changed, from a
sinusoidal timer to keyboard state, exactly matching this task's design goal.

**Root cause:** N/A -- not a CNA gap. A genuinely invented NOXNA control
scheme, since (unlike AccelerometerSample) the original ships no interactive
fallback of any kind to promote.

**Tracked in:** DEFERRED.md item #15 (this sample recorded there as the
"genuinely invented" case, contrasted with AccelerometerSample's "promoted
existing branch" case).

## Vertex format: dummy `(0,0)` texture coordinates instead of a texture-less type

**XNA behaviour:** `GeometricPrimitive.cs`/`SpherePrimitive.cs` build their
procedural sphere geometry using the sample's own, texture-less
`VertexPositionNormal` struct (`VertexPositionNormal.cs`, its own
`IVertexType`) -- position + normal only, no UV data, since there's nothing to
texture-map a procedurally generated sphere onto in this sample (`DiffuseColor`
alone provides the per-ball tint).

**CNA port behaviour:** CNA has no texture-less normal-lit vertex format
(DEFERRED.md item #5's still-open remainder, exactly as this task's own brief
predicted -- this sample hits precisely the same gap Primitives3D's own
`missing.md` already documents, for the same reason: procedurally-generated
lit primitives with no UV data). Per item #5's own recommended workaround,
`src/GeometricPrimitive.hpp::AddVertex()` assigns a dummy, unused
`Vector2(0, 0)` texture coordinate to every vertex and uses the already-proven,
already-working `VertexPositionNormalTexture` + `BasicEffect` lit path instead
of adding a new CNA vertex type. `VertexPositionNormal.cs` itself is not
ported -- there is nothing left for it to do once
`VertexPositionNormalTexture` stands in for it. Confirmed this typed path
(`VertexBuffer::SetData(const VertexPositionNormalTexture*, count)`) does
**not** hit DEFERRED.md item #26's vtable/stride-collision bug: that bug is
specific to `ModelTypeReader::Read()`'s own byte-stride comparison against
`sizeof()` of CNA's (now `IVertexType`-inheriting, vtable-inflated) vertex
structs -- this port never goes through `ModelTypeReader`/`Content.Load<Model>`
at all (the geometry is generated procedurally at runtime, never touching
`.model.json`), and `VertexBuffer::SetData()`'s typed overload manually packs
a plain, vtable-free 32-byte `GpuVertex` struct field-by-field before handing
it to the backend (confirmed by direct source read of
`cna/src/Microsoft/Xna/Framework/Graphics/VertexBuffer.cpp`), so the ambiguity
item #26 describes never arises here.

**Root cause:** CNA has no texture-less normal-lit vertex format (unchanged
from Primitives3D's own finding).

**Tracked in:** DEFERRED.md item #5's still-open remainder (not duplicated as
a new item -- this is a second confirmed sighting of the exact same gap,
resolved with the exact same recommended workaround).

## Confirmed, not a bug: the balls render as near-black spheres with only a white specular highlight

**Finding:** Live screenshots show every ball rendering as a near-black sphere
with a small, bright white specular highlight -- the per-ball `DiffuseColor`
tint (Red/Green/Blue/White/Black, cycled via `ballColors`) is never visibly
distinguishable between balls.

**Root-caused by reading the real XNA/FNA reference source, not assumed to be
a CNA or porting bug:** `GeometricPrimitive.cs`'s `InitializePrimitive()`
enables `basicEffect.DirectionalLight0` and sets its `Direction`, but **never
sets `DirectionalLight0.DiffuseColor`** anywhere in the whole file (confirmed
by a full-file grep). Reading FNA's own
`src/Graphics/Effect/StockEffects/DirectionalLight.cs` (the authoritative XNA
4.0 behavioral reference per `CLAUDE.md`) confirms a freshly-constructed
`DirectionalLight`'s `DiffuseColor` defaults to `Vector3.Zero` (black) --
`EnableDefaultLighting()` is the only path that assigns a non-zero diffuse
color to any light, and this sample's own `GeometricPrimitive.cs` never calls
it (it configures `DirectionalLight0` by hand instead). `cna`'s own
`DirectionalLight.cpp` constructor matches this exactly
(`diffuseColor_(Vector3::Zero)`, confirmed by direct source read). Since the
lit shader's diffuse term is `materialDiffuse * lightDiffuse` and
`lightDiffuse` is genuinely zero here, the ball's own `DiffuseColor` (the
per-ball tint) contributes nothing to the final pixel color regardless of
what it's set to -- only the separately-configured specular term
(`SpecularColor = Vector3.One` on both the light and the material,
`SpecularPower = 32`) is visible, producing exactly the near-black-sphere-
with-white-highlight look confirmed live. **This is the real XNA sample's own
actual rendering result, not a CNA gap or a porting mistake** -- this port
reproduces `GeometricPrimitive.cs`'s configuration field-for-field (see
`src/GeometricPrimitive.hpp::InitializePrimitive()`), so it reproduces this
same (very likely unintentional, upstream-authored) visual characteristic
byte-for-byte.

A related, second dead-code characteristic in the same vein:
`TiltPerspectiveSample.cs`'s `Draw()` sets
`worldGeometry.BasicEffect.DirectionalLight0.Direction = lightDirection;`
(the accelerometer-driven "light always comes from the actual ceiling"
comment) immediately before calling `worldGeometry.Draw(...)` -- but
`DebugDraw.cs`'s own `Draw()` method unconditionally resets
`BasicEffect.DirectionalLight0.Direction = -Vector3.UnitZ;` on every call,
silently overwriting whatever the caller just set. The box's lighting
therefore never actually reflects device tilt in the original either -- only
its fixed ambient term (`AmbientLightColor = (0.5, 0.5, 0.5)`, also never
tilt-driven) lights it. This port reproduces both lines exactly
(`TiltPerspectiveGame::Draw()` sets the light direction, `DebugDraw::Draw()`
immediately overwrites it), matching the original's own inert code path.

**Root cause:** N/A -- confirmed, faithful reproduction of the original
sample's own (likely unintentional) lighting configuration; not a CNA gap.

**Tracked in:** not planned -- nothing to fix; this is what the original does.

## Touch-to-recalibrate substituted with "hold left mouse button"

**XNA behaviour:** `Update()` recalibrates the perspective's "level" reference
every frame the screen is being touched: `if (TouchPanel.GetState().Count > 0)
{ referenceDown = Vector3.Normalize(accelerometer.SmoothAcceleration); }` --
continuously, not just on the initial touch-down, so holding a finger on the
screen continuously tracks whatever tilt is currently held as the new "level."

**CNA port behaviour:** This desktop build has no touchscreen, and a direct
read of `cna/src/CNA/Internal/Input/SdlInputBridge.cpp` confirms CNA's
`TouchPanel` is fed exclusively from real SDL finger events
(`SDL_EVENT_FINGER_DOWN`/`_MOTION`/`_UP` -> `TouchPanel::INTERNAL_onTouchEvent`)
-- mouse button events are never forwarded into it. A literal port of the
`TouchPanel.GetState().Count > 0` check would therefore be permanently
unreachable on this platform. Substituted with `Mouse::GetState().
getLeftButtonProperty() == ButtonState::Pressed`, matching the original's own
"continuously while held" semantics (re-checked every frame, not just on the
rising edge) -- the same touch-to-mouse substitution pattern already
established elsewhere in this repo (e.g. Graphics3D's touch buttons ported to
mouse; see NEXT.md section 6's input-fallback pattern table).

**Root cause:** Platform limitation (no touchscreen on this desktop) combined
with CNA's `TouchPanel` correctly modeling only real touch hardware, not a
CNA gap to fix.

**Tracked in:** not planned.

## Fullscreen / screensaver / GamerServices settings omitted

**XNA behaviour:** The constructor sets `graphics.IsFullScreen = true` and
`Guide.IsScreenSaverEnabled = false` -- Windows-Phone-specific settings.

**CNA port behaviour:** Kept windowed, and `Guide.IsScreenSaverEnabled` is not
ported at all (no desktop screensaver-suppression equivalent is needed for a
short-lived sample process) -- matching every other phone sample in this repo
(Yacht, Bounce, SnowShovel, AccelerometerSample all document the same
omission). The 30 fps fixed timestep **is** kept faithfully
(`setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333))`).

**Root cause:** Desktop dev-loop practicality; matches established repo-wide
precedent.

**Tracked in:** not planned.

## Escape key added as a desktop convenience to exit

**XNA behaviour:** Only `GamePad.GetState(PlayerIndex.One).Buttons.Back ==
ButtonState.Pressed` exits the game.

**CNA port behaviour:** The gamepad Back-button check is ported unchanged,
plus `Keys::Escape` is also checked (a CNA addition, not in the original) --
matching every other phone-sample port in this repo (AccelerometerSample,
Bounce, Yacht, SnowShovel all add the same convenience, since a desktop
keyboard has no physical Back button).

**Root cause:** Desktop usability; matches established repo-wide pattern.

**Tracked in:** not planned.

## `RasterizerState.MultiSampleAntiAlias` omitted

**XNA behaviour:** `BallSimulation.Draw()` constructs a fresh `RasterizerState`
each frame with `MultiSampleAntiAlias = true` (plus the already-default
`FillMode.Solid`/`CullMode.CullCounterClockwiseFace`) before drawing the balls,
then resets to a plain default-constructed `RasterizerState` afterward.

**CNA port behaviour:** Uses `RasterizerState::CullCounterClockwise` directly
(the built-in XNA-default preset) both before and after drawing the balls, per
`CLAUDE.md`'s guidance to use the real state-object presets rather than
constructing ad hoc instances. CNA's `RasterizerState` has no distinct
MSAA-only preset to reach for, and the fill/cull mode is identical to the
default either way, so the only thing not reproduced is the (redundant, since
it doesn't change per-frame) MSAA toggle itself.

**Root cause:** Not investigated whether CNA's `RasterizerState` even exposes
a settable `MultiSampleAntiAlias`-equivalent property beyond the constructor
defaults; out of scope for this port (this sample's own visual result did not
depend on it in live testing).

**Tracked in:** not planned.

## Explicit double-`Update()` of the accelerometer component -- faithfully reproduced original quirk

**XNA behaviour:** `ParallaxSample.Update()` calls `accelerometer.Update(gameTime);`
explicitly at the very top, then calls `base.Update(gameTime)` at the end --
which (since `accelerometer` is also registered in `Components` with
`Enabled == true` on the `Sensor == null` path this port always takes) causes
`AccelerometerHelper.Update()` to run a **second** time in the same frame via
the normal `Components` iteration.

**CNA port behaviour:** Reproduced exactly -- `TiltPerspectiveGame::Update()`
calls `accelerometer_->Update(gameTime)` explicitly, then `Game::Update(gameTime)`
(which iterates `Components`, including `accelerometer_`) runs it again.
This is harmless for this port's keyboard-driven substitute, since each call
just re-samples the current `KeyboardState` idempotently (same key state in,
same result out) -- but it's worth noting explicitly that this same quirk
would have made the *original's own* time-accumulating fake wobble
(`FakeRollTheta += elapsedSeconds * FakeRollSpeed`) silently advance twice as
fast as intended, had anyone ever profiled it. Not fixed here, since faithfully
reproducing the original's own code shape (per `CLAUDE.md`'s porting
philosophy) takes priority, and it causes no observable problem for this port's
own keyboard-driven design.

**Root cause:** N/A -- a genuine quirk in the original sample's own code, not
introduced by this port.

**Tracked in:** not planned.

## `GeometricPrimitive`'s shadow-effect path is dead code -- faithfully reproduced

**Finding:** `GeometricPrimitive.cs`'s `Draw(Matrix, Matrix, Matrix, Color,
bool drawShadow)` overload branches on `drawShadow` to pick between
`basicEffect` and `basicEffectShadow` -- but every call site in
`BallSimulation.Draw()` (both the regular ball draw and the squashed-shadow
draw) passes a literal `false`. `basicEffectShadow` is constructed in
`InitializePrimitive()` but never actually selected anywhere in this sample.

**CNA port behaviour:** Reproduced exactly -- `GeometricPrimitive::Draw(world,
view, projection, color, drawShadow)` is ported with the same `bool` parameter
and the same always-`false` call sites in `BallSimulation::Draw()`. The
"shadow" effect visible on screen (balls squashed flat and rendered in black)
comes entirely from the `Matrix::CreateShadow()`-flattened world matrix passed
to the *regular* (non-shadow) `basicEffect`, not from `basicEffectShadow`
itself.

**Root cause:** N/A -- dead code in the original sample itself, faithfully
reproduced, not a CNA gap.

**Tracked in:** not planned.

## F1 help overlay: no Sample Controls table exists in the original `.htm`

**XNA behaviour:** N/A (CNA-only addition per `CLAUDE.md`).

**CNA port behaviour:** `TiltPerspectiveSample.htm` has no `<table>`-based
"Sample Controls" section at all (confirmed by direct grep) -- only a single
descriptive paragraph: "Touch the screen while the screen is facing directly
toward you to reset the view angle, and then tilt the phone to change the view
angle." This is the same situation Graphics3D's and MarbleMaze's own
`missing.md` already documented (a touch-only or narrative-only original with
nothing for `tools/gen_help_png.py`'s table scraper to extract). Following
that established precedent, `Content/help.png` was generated with a one-off
scratch script (not committed to `tools/`) that imports `gen_help_png.py`'s own
`build_text()`/`render_png()` helpers directly with hand-written text
describing this port's actual control scheme (the keyboard-tilt substitute
above, the mouse-hold recalibration substitute, and Back/Esc to exit).

**Root cause:** The original sample is touch/tilt-only with no keyboard/
gamepad table to scrape -- not a CNA or tooling gap.

**Tracked in:** not planned (same precedent as Graphics3D/MarbleMaze).

## Asset conversion

`stone4.tga` (256x256 RGB Targa, the box's interior wall texture) was converted
directly to PNG via ImageMagick (`convert stone4.tga stone4.png`) -- the same
straightforward `.tga`->`.png` path already established for
ClientServerSample's `Tank.tga`/`Turret.tga` and AimingSample's own assets. No
other content assets exist for this sample (`Background.png`/`GameThumbnail.png`/
`Game.ico` in the original's project directory are Visual Studio project
chrome, not `Content/`-loaded assets, and were not copied).

## Verification

Built via `cmake --build cmake-build-debug --target TiltPerspective_cna_samples
-j$(nproc)` -- 0 errors, 0 warnings, confirmed via two from-scratch object-file
rebuilds of this target (one before, one after removing the temporary
debug-auto-trigger code described below), the second immediately re-run
afterward with no source changes (`ninja: no work to do`). The full aggregate
build (`cmake --build cmake-build-debug -j$(nproc)`) also reports `ninja: no
work to do` afterward, confirming this sample's addition to the root
`CMakeLists.txt` doesn't break the aggregate build.

Ran under `SDL_VIDEODRIVER=x11` for 8+ seconds across three separate runs with
no crash (confirmed via `ps aux` and no error output in the captured stdout/
stderr log beyond the normal EasyGL backend banner). Window (`480x800`, title
`"Game"`) confirmed via `xwininfo -root -tree | grep -i tiltperspective`.

Screenshots with real (unmodified) keyboard/mouse state confirm: the box's
stone texture renders correctly (ambient-lit, per the dead-code lighting note
above); all 25 balls render as shaded spheres (near-black body + white
specular highlight, per the confirmed-not-a-bug finding above) and visibly
change position/clustering between two screenshots taken several seconds
apart under passive (level, untilted) input -- direct confirmation that
`BallSimulation`'s gravity/collision physics runs live every frame, not just
once at startup.

Live keyboard-tilt input verification hit this repo's own known `xdotool`
shared-desktop caveat (NEXT.md section 5): `xdotool getactivewindow` showed a
different real user window (`0x400003`, decimal `4194307`) held focus
throughout. Per this repo's established fallback, the code was temporarily
patched in two places (both reverted before commit, re-verified with a clean
from-scratch rebuild afterward -- 0 warnings, confirmed a second time):

1. `AccelerometerHelper::Update()`'s synthesized `stateValue.X` forced `+= 0.8f`
   (simulating "Right arrow held") right before `Normalize()` -- rebuilt, ran,
   and screenshotted: the ball cluster visibly shifted toward the tilted
   direction and settled against the corresponding wall, and the box's
   perspective framing visibly changed between the untilted and tilted runs --
   direct proof the keyboard-tilt vector actually drives both
   `BallSimulation`'s gravity (`IAccelerometerService::RawAcceleration`) and
   `TiltPerspectiveGame::ComputeEyeVector()`'s perspective shift
   (`SmoothAcceleration`), exactly mirroring how the original's own fake wobble
   would have driven both, just from keyboard state instead of a sine wave.
2. `TiltPerspectiveGame::Update()`'s F1-overlay branch forced `helpTimer_ =
   10.0f` on the very first frame -- rebuilt, ran, and screenshotted: the first
   screenshot (~2 s in) shows the (clipped, see below) help text visible over
   the scene; a later screenshot (taken after enough real wall-clock time --
   including this session's own tool-call overhead -- had elapsed for the 10 s
   game-time timer to expire) shows the overlay gone and the scene still
   correctly rendered underneath, confirming the F1 overlay's own timer
   countdown logic works unmodified from `CLAUDE.md`'s standard pattern.

`TiltPerspectiveSample.htm` has no Sample Controls table (see above), so
`Content/help.png` was generated via a one-off scratch script rather than the
standard `tools/gen_help_png.py` path; the resulting overlay (822x192) is wider
than this sample's 480px-wide portrait window, so the centered text is
left/right-clipped -- the same pre-existing, already-accepted
F1-overlay-width-vs-portrait-window characteristic already documented in
`samples/Yacht/missing.md` and `samples/AccelerometerSample/missing.md`, not a
new or sample-specific issue.

Both temporary patches were reverted before committing, and the target was
rebuilt from scratch afterward to reconfirm 0 warnings (see above) -- the
committed source contains neither.

No new DEFERRED.md item was needed: the one genuinely new-to-this-sample gap
(texture-less procedural-primitive vertices) is already fully covered by
DEFERRED.md item #5's existing "still open" remainder, and every other finding
above is either a faithful, confirmed-via-source-read reproduction of the
original sample's own code (including two inert/dead-code characteristics) or
an already-established repo-wide pattern (fullscreen/screensaver omission,
Escape-to-exit, touch-to-mouse substitution, the F1-overlay-width-vs-portrait-
window characteristic, and the "no Sample Controls table" one-off help-script
precedent).
