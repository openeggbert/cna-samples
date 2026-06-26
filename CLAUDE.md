# CNA Samples — Claude Code Guidelines

## What this repository is

C++ ports of the official **Microsoft XNA Game Studio 4.0** sample collection,
running on [CNA](../cna) — a C++ reimplementation of XNA 4.0 built on SDL3.

The original C# samples live at `/rv/tmp/XNAGameStudio/Samples` (local mirror; `/rv/tmp` persists across reboots unlike `/tmp`).
FNA source at `/rv/data/library/github.com/FNA-XNA/FNA` is the authoritative
XNA 4.0 API behavioral reference.

---

## Porting philosophy — stay close to the C# original

Each C++ port must be **as similar as possible to the XNA 4.0 C# original**,
within the natural constraints of the C# → C++ language difference
(no GC, no properties, no `using` blocks, value/reference semantics, etc.).
Deviation beyond those constraints needs a concrete reason.

**Prefer sharp-runtime types over raw C++ equivalents:**

| C# (.NET) | sharp-runtime C++ | Do NOT use |
|---|---|---|
| `new Random()` | `System::Random random;` | `std::mt19937 rng(42)` |
| `random.Next(min, max)` | `random.Next(min, max)` | `std::uniform_int_distribution` |
| `random.NextDouble()` | `random.NextDouble()` | `std::uniform_real_distribution` |
| `List<T>` | `std::vector<T>` | — (no sharp-runtime List yet) |
| `Math.Cos(x)` | `std::cos(x)` | — (no System::Math yet) |

Include sharp-runtime types with: `#include "System/Random.hpp"` etc.
The `SHARP_RUNTIME` CMake target exposes its `include/` directory publicly.

---

## CNA Property Access Patterns

CNA uses a `DEF_PROP` macro that generates `getXxxProperty()` / `setXxxProperty()`
methods — **not** public member variables.

Key examples that often trip up porting:

| XNA C# | CNA C++ |
|---|---|
| `device.Viewport.Width` | `device.getViewportProperty().getWidthProperty()` |
| `device.Viewport.Height` | `device.getViewportProperty().getHeightProperty()` |
| `graphics.GraphicsDevice` | `graphics.getGraphicsDeviceProperty()` (returns pointer) |
| `graphics.PreferredBackBufferWidth = 800` | `graphics.setPreferredBackBufferWidthProperty(800)` |
| `Content.RootDirectory = "Content"` | `getContentProperty().setRootDirectoryProperty("Content")` |
| `effect.CurrentTechnique.Passes[0].Apply()` | `effect.getCurrentTechniqueProperty()->getPassesProperty()[0].Apply()` |
| `gameTime.TotalGameTime.TotalSeconds` | `gameTime.getTotalGameTimeProperty().getTotalSecondsProperty()` |
| `color.R / 255.0f` | `color.getRProperty() / 255.0f` |
| `mouse.LeftButton` | `mouse.getLeftButtonProperty()` |
| `mouse.X` / `mouse.Y` | `mouse.getXProperty()` / `mouse.getYProperty()` |
| `gamePad.Buttons.Back` | `gamePad.IsButtonDown(Buttons::Back)` |
| `Color(byte, byte, byte)` | `Color(int, int, int, 255)` |
| `device.BlendState = BlendState.AlphaBlend` | `device.setBlendStateProperty(BlendState::AlphaBlend)` |
| `device.DepthStencilState = DepthStencilState.Default` | `device.setDepthStencilStateProperty(DepthStencilState::Default)` |
| `device.RasterizerState = RasterizerState.CullCounterClockwise` | `device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise)` |
| `effect.GraphicsDevice` | `effect.getGraphicsDeviceInternal()` |

## Game class requirements

Every class derived from `Microsoft::Xna::Framework::Game` **must** implement:

```cpp
const std::string& GetTypeName() const override {
    static const std::string name = "YourGameClassName";
    return name;
}
```

## GraphicsDevice state

CNA exposes the full XNA-style property setters — use them directly:

```cpp
device.setBlendStateProperty(BlendState::AlphaBlend);   // or ::Opaque
device.setDepthStencilStateProperty(DepthStencilState::Default);
device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
// wireframe: set FillMode::WireFrame + CullMode::None on a RasterizerState instance
```

Do **not** use the NOXNA helpers `SetBlendEnabled` / `SetDepthTestEnabled` /
`SetDepthWriteEnabled` — they exist but bypass the XNA state objects.

## Assets

XNA `.xnb` files are **never** supported by CNA and will not be.
When porting a sample that loads assets via `Content.Load<T>()`:

- **Textures** (`.xnb` → `Texture2D`): convert the source art to PNG and place in `Content/`.
- **Audio** (`.xnb` → `SoundEffect`/`Song`): convert to OGG or WAV and place in `Content/`.
- **Models** (`.xnb` → `Model`): convert to glTF and place in `Content/`.
- **SpriteFont** (`.xnb` → `SpriteFont`): deferred — CNA has no SpriteFont yet (see `DEFERRED.md`).
  Omit `DrawString` calls and the `Content.Load<SpriteFont>` call from the port.
- **Effects** (`.xnb` → custom `Effect`): deferred until CNA supports user shaders.

Source art is usually in the XNA sample's `*Content/` directory or next to the `.xnb` files.

## Sample documentation

Every sample directory must contain the original **`SampleName.htm`** documentation file,
copied verbatim from `/rv/tmp/XNAGameStudio/Samples/SampleName_4_0/SampleName.htm`.
Copy it to `samples/SampleName/` (next to `src/` and optionally `Content/`).

## Adding a new sample

1. Create `samples/SampleName/` with `src/` and optionally `Content/`.
2. Copy `SampleName.htm` from `/rv/tmp/XNAGameStudio/Samples/SampleName_4_0/` to `samples/SampleName/`.
3. Create `samples/SampleName/missing.md` — see **Sample missing.md** below.
4. Add `cna_add_sample(sample_name SOURCES src/Program.cpp ...)` in `CMakeLists.txt`.
5. Uncomment the matching `add_subdirectory(samples/SampleName)` in root `CMakeLists.txt`.
6. Implement `GetTypeName()` in every `Game` subclass.
7. Use `getXxxProperty()` / `setXxxProperty()` for all property access — no direct
   member access.
8. Convert any XNB assets to open formats (see Assets section above).
9. See `DEFERRED.md` for known gaps that may require CNA changes.

## Sample missing.md

Every sample directory **must** contain a `missing.md` file that documents all
known differences between the CNA C++ port and the XNA 4.0 C# original.

Format:

```markdown
# Missing / Differences from XNA 4.0 original

## <Short title of difference>
**XNA behaviour:** …
**CNA port behaviour:** …
**Root cause:** …
**Tracked in:** DEFERRED.md item N  (or "CNA issue", "not planned", etc.)
```

Write one section per difference.  If the port is fully faithful, write
`No known differences.` so it is clear the file was reviewed.

## Related projects

- `../cna` — CNA framework (build dependency)
- `../sharp-runtime` — .NET BCL types (transitive dependency via CNA)
- FNA reference: `/rv/data/library/github.com/FNA-XNA/FNA`
