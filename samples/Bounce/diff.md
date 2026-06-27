# Differences from the XNA 4.0 Original

## Platform

The original sample targets **Windows Phone 7** exclusively (fullscreen, 30 fps,
portrait/landscape orientation, touch input). This port runs on the **desktop**
(windowed, 60 fps, keyboard/gamepad input).

## Accelerometer input

| | Original (Phone) | CNA port (Desktop) |
|---|---|---|
| Input source | Hardware accelerometer via `Microsoft.Devices.Sensors.Accelerometer` | Arrow keys on keyboard simulate tilt; real accelerometer used automatically if the platform supports it (Android, mobile web) |
| Left / Right tilt | Physical device tilt along Y axis | ← / → arrow keys |
| Forward / Back tilt | Physical device tilt along Z axis | ↑ / ↓ arrow keys |
| Shake to bounce | Detected from accelerometer magnitude peak | Not available on desktop (no shake input) |
| Orientation compensation | Y axis negated in `LandscapeLeft` mode | Skipped (desktop has no display orientation) |

## Tilt indicator (desktop addition)

A small overlay is drawn in the top-left corner of the screen to show the
current tilt state. It has no equivalent in the original phone version.

- **Gray box** — the full tilt range (±limit in both axes)
- **Gray crosshair** — the neutral centre (gravity straight down)
- **Gray dot at centre** — marks the zero-tilt position
- **Yellow dot** — current simulated tilt (moved with arrow keys)
- **Cyan dot** — shown instead of yellow when a real hardware accelerometer
  is active (e.g. on Android)

## Vertex format

The original uses a custom `VertexPositionNormal` type (24 bytes: position +
normal, no UV). The port uses `VertexPositionNormalTexture` (32 bytes) with
UV set to (0, 0). Lighting output is visually identical; the extra 8 bytes
per vertex are unused.

## Other omitted phone settings

- `graphics.IsFullScreen = true` — omitted; port runs windowed.
- `TargetElapsedTime = TimeSpan.FromTicks(333333)` (30 fps cap) — omitted;
  port runs at the default 60 fps.
