# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-09.** Builds with 0 warnings. Ported using CNA's stock
`Model`/`BasicEffect`/`DrawableGameComponent`, no CNA-side gaps for the sample's
own lighting/texture code (the historical blocker below is resolved for
`Model`-based rendering). Confirmed live for 6+ seconds with no crash. The
spaceship itself does not currently render on screen — not a bug introduced by
this port, but the same pre-existing EasyGL near-plane-clipping framework bug
already tracked for other samples, confirmed here via direct isolation testing
(see below). Everything else (starfield background toggle, explosion animation,
all 4 buttons, F1 help overlay) confirmed live via screenshot.

Source: `/rv/tmp/XNAGameStudio/Samples/Graphics3DSample_4_0/Sample3DGraphics/
Sample3DGraphics/{GameMain.cs, Models/Spaceship.cs, Animation/Animation.cs,
Buttons/{Clickable.cs, Checkbox.cs}}`. (`Buttons/Button.cs` is dead code in the
original — namespace mismatch with the rest of the sample, never instantiated
by `GameMain.cs` — and was not ported.)

## Asset conversion: `spaceship.fbx` is an old binary FBX (v6.1/6000), unreadable by this repo's normal tools
**XNA behaviour:** the content pipeline's FBX importer (built on the full
Autodesk FBX SDK) reads `Models/spaceship.fbx` directly, regardless of its FBX
version.

**CNA port behaviour:** neither of this repo's normal conversion tools handles
this specific file: `assimp` (5.4) and Blender (4.3.2)'s FBX importer both
explicitly refuse it (`"Version 6000 unsupported, must be 7100 or later"` /
`"FBX-DOM unsupported, old format version, supported are only FBX 2011, FBX
2012 and FBX 2013"`) — confirmed live, this file is `Kaydara FBX Binary`
version 6000 (i.e. FBX 6.1, an old binary format predating the ASCII FBX 6.1
files `tools/fbx_ascii2model.py` already handles, e.g. `tank.fbx`/`terrain.fbx`).
**Worked around** by using the `ufbx` Python binding (installed in a scratch
virtualenv, not added as a repo dependency — this was a one-off conversion, not
a repeated tool) to load the file, bake each vertex/normal through the mesh's
`node.geometry_to_world` matrix (the model has exactly one mesh, "enemy", and
one hosting node with a real, non-identity 0.05 uniform-scale transform baked
into the node hierarchy — confirmed via the file's raw vertex bounding box
(±1200 local units) vs. `ufbx`'s reported world transform (±60 units after
scaling), so this is a genuine authored transform, not a `ufbx` artifact),
flip V (FBX/Maya's bottom-origin UV convention vs. XNA/D3D's top-origin one),
and write a plain Wavefront `.obj`. That `.obj` was then converted with the
existing, already-reviewed `tools/obj2model.py` — no new converter script was
added to `tools/`, since this was a one-off asset, not an established
recurring format this repo needs to support again.
**Root cause:** `tools/fbx_ascii2model.py` only ever supported ASCII FBX;
`spaceship.fbx` predates that and is binary. Not a CNA gap — a content-tooling
gap, and a narrow one (this is the first and, so far, only binary FBX 6.1 file
encountered while porting).
**Tracked in:** not filed as a DEFERRED.md item — one-off asset-conversion
problem, not a recurring CNA framework gap. If a future sample needs another
old-binary-FBX asset, this port's approach (`ufbx` → `.obj` →
`tools/obj2model.py`) is the precedent to reuse.

## CNA framework gap found: `Game::DoInitialize()` wires up `ComponentAdded` *after* calling the user's own `Initialize()` override
**XNA/FNA behaviour:** `Game`'s constructor subscribes to
`Components.ComponentAdded` immediately, before any override of `Initialize()`
can run. This is why real XNA supports (and this sample's own C# original
relies on) creating `DrawableGameComponent`s and calling `Components.Add(...)`
from *inside* `Initialize()` (`GameMain.cs`'s `CreateLightEnablingButtons()`
etc., all called from `Initialize()`) — each added component's own
`Initialize()`/`LoadContent()` fires synchronously as part of `Add()`.

**CNA port behaviour:** confirmed live (this port originally segfaulted on the
very first `Draw()` before this was found) — `cna`'s `Game::DoInitialize()`
(`Game.cpp`) calls the user's `Initialize()` override first, and only wires up
`Components_.ComponentAdded`/`ComponentRemoved` afterward. A `Checkbox`
(`DrawableGameComponent`) added to `Components` from within this port's own
`Initialize()` override — the direct C++ translation of the original's own
pattern — is added to the collection (so it's later categorized as
updateable/drawable and does run each frame) but its own `Initialize()`
(and therefore `LoadContent()`, which creates its `SpriteBatch`/loads its
texture) is **never called**, since the event that would trigger it isn't
subscribed yet. The result: `Checkbox::Draw()` dereferences a value-less
`std::optional<SpriteBatch>`/`std::optional<Texture2D>` — undefined behavior,
observed as a segfault on the first frame.

**Root cause:** `Game::DoInitialize()` (`cna`'s `src/Microsoft/Xna/Framework/
Game.cpp`) subscribes `Components_.ComponentAdded`/`ComponentRemoved` after
calling `Initialize()`, not before — a real deviation from FNA/XNA's own
`Game` constructor, where the equivalent subscription happens immediately.

**Workaround applied (this sample):** a small `AddComponent(Checkbox*)` helper
in `Graphics3DGame` that calls `getComponentsProperty().Add(component)`
followed by an explicit `component->Initialize()`. Safe regardless of whether
`cna` is later fixed to call this automatically, since
`DrawableGameComponent::Initialize()` already guards against
double-initialization (`if (!initialized_) { ...; LoadContent(); }`).

**Tracked in:** not yet filed as a DEFERRED.md item — flagging here for the
user to decide whether `cna`'s `Game.cpp` should move the
`ComponentAdded`/`ComponentRemoved` subscription into the constructor (or
earlier in `DoInitialize()`, before calling `Initialize()`) to match real
XNA/FNA. Every other sample in this repo that adds components does so from the
*constructor* (before `Initialize()` even runs), which happens to sidestep this
gap entirely — this is the first sample to add components from `Initialize()`
itself, which is why it's the first to surface this.

## CNA gap (pre-existing, not introduced by this port): near-plane-clipping-family bug renders the spaceship as fully invisible, not even a thin line
**Confirmed live, extensively isolated this session:**
- The model (converted per above), its vertex/index buffers, and the
  World/View/Projection matrices computed by this port's `Spaceship::Draw()`
  are all individually correct — verified by hand: a sample vertex transforms
  to NDC `(0.015, -0.003, 0.998)` (dead center of screen, within the visible
  volume) using the exact matrices this port computes for the C# original's own
  camera setup (`(3500,400,0)+(0,250,0)` eye, `(0,250,0)` target, 45° FOV,
  near=10, far=20000 — all copied verbatim from `GameMain.cs`).
- Swapping in the already-proven `tank.model.json` asset (from
  `CustomModelClass`) through this exact same drawing code, at this exact same
  camera distance (~3523 units), **also renders nothing** — ruling out the
  spaceship asset conversion as the cause.
- Re-testing with `tank.model.json` at `CustomModelClass`'s own, much closer
  camera distance (~1059 units) **reproduces the known thin-line artifact**
  (`NEXT.md` section 4) through this same code path — confirming the drawing
  code itself is correct, and that the same underlying EasyGL bug produces two
  different visible symptoms depending on camera distance: a thin line at
  ~1000 units, complete invisibility at ~3500 units.
- Removing all `BlendState`/`RasterizerState`/`DepthStencilState`/
  `SamplerState` overrides, forcing `RasterizerState::CullNone`, and clearing
  the depth buffer explicitly (`GraphicsDevice::Clear(color, depth)` instead of
  the single-arg `Clear(color)` overload, which — separately confirmed — never
  clears depth at all) made no difference either.
- This all points at the same near-plane-clipping-adjacent rendering defect
  already tracked in `NEXT.md` section 4/8 (task 2) and demonstrated by
  `CameraShake`/`CustomModelClass`, now confirmed to also fully hide geometry
  (not just degenerate it to a line) at greater camera distances. Not attempted
  to fix here, per this repo's convention of tracking framework rendering bugs
  centrally rather than re-diagnosing them per sample.
**Tracked in:** `NEXT.md` section 4/8 (task 2), pre-existing; this port adds a
new data point (full invisibility at long camera distance, not just a thin
line) worth folding into that investigation.

## Separately confirmed: `GraphicsDevice::Clear(Color)` (single-argument overload) never clears the depth buffer
**XNA behaviour:** `GraphicsDevice.Clear(Color)` is documented to clear the
color target, depth buffer, and stencil buffer together (equivalent to
`Clear(ClearOptions.Target | ClearOptions.DepthBuffer | ClearOptions.Stencil,
color, 1, 0)`).
**CNA port behaviour:** confirmed via direct source read
(`GraphicsDevice.cpp`) — the single-argument `Clear(const Color&)` overload
only ever calls `backend_->Clear(r,g,b,a)` (color only); it never touches
depth/stencil. The two-argument `Clear(const Color&, float depth)` overload
does clear both (`Clear(Target|DepthBuffer, color, depth, 0)`), and using it in
place of the single-arg overload was tested during this session's
investigation above (see near-plane-clipping entry) — it did not change the
spaceship's invisibility, so depth-buffer staleness is not the cause of that
bug, but the `Clear(Color)` gap is real and independent of it. Every 3D sample
in this repo (`CameraShake`, `CustomModelClass`, `LensFlare`, this one) calls
the single-arg overload, so all of them are — in principle — drawing every
frame against a depth buffer that was never actually cleared, relying on
whatever the driver leaves behind. Reverted back to the single-arg overload
for this port to stay consistent with every other sample's existing pattern,
rather than fixing only this one sample.
**Root cause:** `GraphicsDevice::Clear(const Color&)` in `cna`'s
`GraphicsDevice.cpp` doesn't forward to the `ClearOptions`-based overload with
`DepthBuffer` included.
**Tracked in:** not yet filed as its own DEFERRED.md item — flagging here since
it's a real, confirmed deviation from documented XNA behavior, independent of
the near-plane-clipping investigation above, and affects every 3D sample in
this repo, not just this one.

## Touch → mouse input substitution (no touchscreen on this desktop)
**XNA behaviour:** the original is Windows-Phone-only — `TouchPanel.
EnabledGestures = FreeDrag | Pinch | PinchComplete` drives camera rotation
(drag) and zoom (pinch), and `Clickable`/`Checkbox`'s own `HandleInput()` reads
`TouchPanel.GetState()` directly for the 4 on-screen buttons.
**CNA port behaviour:** this desktop has no touchscreen, so every touch
interaction is substituted with mouse input, per this repo's established
input-fallback conventions (`NEXT.md` section 6): left-mouse-drag accumulates
into the same `rotationXAmount`/`rotationYAmount` the original's `FreeDrag`
delta did; the mouse scroll wheel adjusts `cameraFOV` (substituting for
`Pinch`, using an arbitrary but reasonable scale factor since there's no
established "wheel-equivalent zoom rate" convention in this repo yet); each
`Checkbox`/`Clickable` checks mouse position + left-button-down against its
rectangle instead of `TouchPanel.GetState()`'s first active touch. Also added
`Keyboard::GetState().IsKeyDown(Keys::Escape)` as an additional exit path
alongside the original's gamepad-Back-only check, matching the convention
nearly every other sample in this repo follows (the phone original has no
keyboard at all, so this is a pure desktop-usability addition, not a
correction). Not live-verified via synthetic mouse input this session (only
verified via source-level review and the temporary debug-auto-trigger pattern
for the 4 buttons/background/animation/help-overlay visual paths — see
Verification below); dragging/wheel-zoom specifically were not exercised.
**Root cause:** N/A — platform-appropriate substitution, not a CNA gap.
**Tracked in:** not planned (same category as every other touch→mouse
substitution already made elsewhere in this repo).

## `Buttons/Button.cs` (dead code in the original) not ported
**XNA behaviour:** `Buttons/Button.cs` exists in the original source tree but
is never instantiated anywhere in `GameMain.cs` — it's in a different,
mismatched namespace (`Sample3DGraphics` vs. the rest of the sample's
`Graphics3DSample`) referencing a `Sample3DGraphicsGame` type that doesn't
exist in this sample, confirming it's leftover/unused code, not a
reachable code path.
**CNA port behaviour:** not ported — confirmed via direct grep that nothing in
`GameMain.cs` references `Button` (only `Checkbox`, which has its own,
differently-shaped `IsChecked`/toggle behavior and *is* ported).
**Root cause:** N/A — dead code in the original, not a CNA gap.
**Tracked in:** not planned.

## `3DGraphics.htm` has no keyboard/gamepad control table; custom `help.png` generator used
**XNA behaviour:** the original's "Sample Controls" section is a plain bullet
list of touch gestures/buttons — no `<table>` at all (this being a
touch-only phone sample).
**CNA port behaviour:** `tools/gen_help_png.py`'s generic table-scraper found
nothing to extract (there's no table to find). Used a one-off script (same
pattern already established for `MicrophoneEcho`, per `CLAUDE.md`) that
imports `tools/gen_help_png.py`'s `render_png()` helper directly and supplies
custom text describing this port's own mouse-based control scheme (see the
input-substitution entry above), rather than the original's literal
touch-gesture wording, since the two don't match the actual input handled by
this port.
**Root cause:** N/A — CNA-only tooling accommodation, not a difference in
behavior.
**Tracked in:** not planned.

## Historical blocker (resolved): `BasicEffect.LightingEnabled`/`PreferPerPixelLighting` with 3 directional lights + specular
**XNA behaviour:** `Spaceship.Draw()` configures 3 independent
`DirectionalLight0/1/2`s (diffuse+specular+direction each), `SpecularColor`/
`SpecularPower`, `AmbientLightColor`, `LightingEnabled = true`, and toggles
`PreferPerPixelLighting` from an on-screen button — all through the **stock**
`BasicEffect`, confirmed via source audit to involve zero custom `.fx` shaders.
**CNA port behaviour:** ported directly — `Spaceship::SetEffectLights()` sets
all of the above every frame, `IsPerPixelLightingEnabled` drives
`setPreferPerPixelLightingProperty()`. Not independently visually verified
(the model doesn't render at all — see the near-plane-clipping entry above),
but the effect API calls themselves are all present and confirmed to exist in
CNA's `BasicEffect` (checked directly against its header) — whether
`PreferPerPixelLighting` actually changes the shader used, versus being a
no-op, was **not** determined this session (would need the near-plane-clipping
bug fixed first to see the model at all).
**Root cause (historical):** was a missing lit-shader path for
`VertexPositionNormalTexture` in CNA; resolved for `Model`-based samples
(DEFERRED.md item #5).
**Tracked in:** DEFERRED.md item #5 (resolved for the lighting API surface;
visual correctness pending the near-plane-clipping fix above).

## Verification
**Confirmed live:** built cleanly (0 warnings, after fixing the
`ComponentAdded`-timing segfault above). Ran under `SDL_VIDEODRIVER=x11` for
6+ seconds — process stays up with no crash. Screenshot at the original's own
default state (all 3 lights on, per-pixel/animation/background all off)
matches expectations exactly except for the invisible ship (near-plane-clipping
bug, not this port). Using a temporary debug auto-trigger (forced
`backgroundTextureEnablingButton_`/`animationButton_` checked, forced
`helpTimer_ = 10`, all removed before commit — same established pattern as
other samples in this repo), confirmed live via screenshot: the starfield
background texture renders correctly, the explosion sprite-sheet animation
renders and advances through frames correctly, all 4 buttons render with
correct icon and yellow/white checked-state tinting, and the F1 help overlay
renders centered on top of everything with the correct custom control text.
Mouse-driven drag-rotate and wheel-zoom were not exercised via synthetic input
this session (same `xdotool` reliability caveat noted elsewhere in this repo)
but are straightforward, unconditional code paths with no dependency on the
invisible-model bug above.
