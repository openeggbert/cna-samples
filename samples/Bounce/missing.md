# Missing / Differences from XNA 4.0 original

## Accelerometer orientation check omitted
**XNA behaviour:** When the phone is in `DisplayOrientation.LandscapeLeft` mode the
Y component of the accelerometer reading is negated before use.
**CNA port behaviour:** Orientation check skipped; desktop has no display orientation.
**Root cause:** Desktop-specific scope decision.
**Tracked in:** not planned.

## Keyboard-simulated accelerometer uses a different input model
**XNA behaviour:** The original's `Accelerometer` helper (`Accelerometer.cs`) only
ever falls back to keyboard simulation inside the `WINDOWS_PHONE` emulator branch,
which recomputes a fresh reading every frame: `Z = -1`, `X`/`Y` each nudged by
exactly ±1 per currently-held arrow key, then the whole vector is `Normalize()`d.
Releasing all keys snaps the simulated tilt back to level (X=0, Y=0) immediately.
**CNA port behaviour:** `BounceGame` falls back to its own `simAccel_` state
whenever the real `Accelerometer::Start()` throws (always true on typical desktop
hardware without an accelerometer). Held arrow keys *increment/decrement* `simAccel_.Y`
(left/right) and `simAccel_.Z` (up/down) by a `step` of 0.03 per frame, clamped to
`[-tiltLimit, tiltLimit]` (Y) / `[-tiltOffset-tiltLimit, -tiltOffset+tiltLimit]` (Z).
Releasing all keys leaves the tilt exactly where it was — it does not reset to level.
**Root cause:** Deliberate desktop UX choice (gradual joystick-like tilt is easier to
control with a keyboard than an instant on/off tilt that snaps back to level every
frame); CNA's `Accelerometer` also models a real polled sensor rather than an
emulator-only fake, so there is no direct equivalent to port faithfully.
**Tracked in:** not planned.

## Floor-collision "shake" impulse reordered relative to gravity integration
**XNA behaviour:** `UpdateSpheres` applies the shake impulse *after* this frame's
position/velocity integration, inside the floor-penetration branch
(`if (mySphere.Position.Y < floorPlaneHeight + mySphere.Radius)`), and first undoes
this frame's gravity contribution (`mySphere.Velocity -= gravity * elapsedGameTime;`)
before evaluating the "come to rest" / bounce branch. The shake speed clamp is also
bugged in the original: `speedadjust = Math.Min(speed, 4.0f);` is immediately
overwritten by `speedadjust = Math.Max(speed, 2.0f);`, so the effective clamp is
unbounded above (`Math.Max(speed, 2.0f)`), not `[2, 4]`.
**CNA port behaviour:** `UpdateSpheres` applies the shake impulse in a separate pass
*before* this frame's integration, gating on the **previous** frame's resting
position with a `+0.01f` tolerance (`s.Position.Y <= floorPlaneHeight + s.Radius + 0.01f`)
rather than the post-integration exact position test. It does not undo the current
frame's gravity contribution before the rest/bounce check. The speed clamp is
properly bounded: `adj = std::min(std::max(speed, 2.0f), 4.0f)` (i.e. `[2, 4]`),
correcting the original's dead-code clamp bug.
**Root cause:** Refactor for clarity during the port; the `[2, 4]` clamp is an
intentional fix of what looks like an unintentional bug in the C# original. Net
effect is a one-frame timing shift in when the shake impulse lands and slightly
different velocities entering the floor bounce/rest branch; visually indistinguishable
during normal play.
**Tracked in:** not planned.

## VertexPositionNormal replaced with VertexPositionNormalTexture
**XNA behaviour:** Uses a custom `VertexPositionNormal` vertex type (24 bytes:
position + normal, no texture coordinate).
**CNA port behaviour:** Uses `VertexPositionNormalTexture` (32 bytes) with UV set
to (0, 0). Lighting output is visually identical; the extra 8 bytes are unused.
**Root cause:** CNA `VertexBuffer::SetData` has no overload for custom vertex types;
`VertexPositionNormalTexture` is the closest built-in type with the same
Position/Normal fields.
**Tracked in:** CNA issue (typed SetData overloads).

## Fullscreen and 30 fps target omitted
**XNA behaviour:** `graphics.IsFullScreen = true` and
`TargetElapsedTime = TimeSpan.FromTicks(333333)` (30 fps) — phone-specific settings.
**CNA port behaviour:** Windowed, default 60 fps.
**Root cause:** Phone-specific settings outside the scope of a desktop port.
**Tracked in:** not planned.

## Color names replaced with RGBA literals
**XNA behaviour:** Uses `Color.Red`, `Color.Green`, `Color.Blue`, `Color.White`,
`Color.Black`, `Color.CornflowerBlue`.
**CNA port behaviour:** Equivalent RGBA values used directly (e.g.
`Color(100, 149, 237, 255)` instead of `Color::CornflowerBlue`). Rendered output
is identical.
**Root cause:** Stylistic leftover from before CNA gained named color constants;
CNA's `Color` class now exposes the full XNA/.NET named palette (`Color::Red`,
`Color::CornflowerBlue`, etc. — see `Color.hpp`), so this is no longer a CNA
limitation, just an un-migrated literal in this sample's source.
**Tracked in:** not planned (cosmetic only, no behavioural difference).

## Directional light diffuse color set explicitly (original leaves it black)
**XNA behaviour:** `GeometricPrimitive.InitializePrimitive()` never sets
`basicEffect.DirectionalLight0.DiffuseColor` and never calls
`EnableDefaultLighting()`. Per FNA's `BasicEffect`/`DirectionalLight`
(`DirLight0DiffuseColor` and `AmbientLightColor` both default to `Vector3.Zero`
until explicitly assigned) and the `ComputeLights` shader (`Lighting.fxh`:
`result.Diffuse = mul(diffuse, lightDiffuse) * DiffuseColor.rgb + EmissiveColor;`),
the diffuse term evaluates to zero for every sphere regardless of its material
color — only the achromatic specular highlight (`SpecularColor = Vector3.One` on
both the effect and `DirectionalLight0`) is visible, so the unmodified original
renders spheres essentially black with a white specular highlight blob.
**CNA port behaviour:** `GeometricPrimitive::InitializePrimitive()` additionally
calls `basicEffect_->getDirectionalLight0Property().setDiffuseColorProperty(Vector3::One)`,
giving the light a full-white diffuse contribution so each sphere renders in its
assigned tint color (red/green/blue/white/black) as apparently intended. This
line was added in commit `0df6f94` ("Fix Bounce: set light DiffuseColor white,
add Accelerometer support") but was never recorded here.
**Root cause:** Deliberate fix for what reads as a missing line/bug in the
original XNA sample, not a scope-driven simplification.
**Tracked in:** not planned (visual fix retained intentionally).
