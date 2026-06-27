# Missing / Differences from XNA 4.0 original

## Accelerometer orientation check omitted
**XNA behaviour:** When the phone is in `DisplayOrientation.LandscapeLeft` mode the
Y component of the accelerometer reading is negated before use.
**CNA port behaviour:** Orientation check skipped; desktop has no display orientation.
**Root cause:** Desktop-specific scope decision.
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
**CNA port behaviour:** Equivalent RGBA values used directly.
**Root cause:** CNA Color does not expose named static constants yet.
**Tracked in:** CNA issue (minor convenience gap).
