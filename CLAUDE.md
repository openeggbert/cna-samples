# CNA Samples — Claude Code Guidelines

## What this repository is

C++ ports of the official **Microsoft XNA Game Studio 4.0** sample collection,
running on [CNA](../cna) — a C++ reimplementation of XNA 4.0 built on SDL3.

The original C# samples live at `/rv/tmp/XNAGameStudio/Samples` (local mirror; `/rv/tmp` persists across reboots unlike `/tmp`).
FNA source at `/rv/data/library/github.com/FNA-XNA/FNA` is the authoritative
XNA 4.0 API behavioral reference.

---

## Porting philosophy — stay close to the C# original

Each C++ port must be **as similar as possible to the XNA 4.0 C# original**.
Deviation is allowed only when forced by a language or API difference.

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

## Game class requirements

Every class derived from `Microsoft::Xna::Framework::Game` **must** implement:

```cpp
const std::string& GetTypeName() const override {
    static const std::string name = "YourGameClassName";
    return name;
}
```

## GraphicsDevice state (NOXNA workarounds)

CNA does not yet expose XNA-style `GraphicsDevice.BlendState` or
`GraphicsDevice.RasterizerState` property setters.  Use:

```cpp
device.SetDepthTestEnabled(bool);
device.SetBlendEnabled(bool);
device.SetDepthWriteEnabled(bool);
```

These are `NOXNA` extensions (not in XNA 4.0 public API).

## Assets

XNA `.xnb` files are NOT supported.  See `DEFERRED.md` item 1 for the
asset conversion strategy.  Place raw assets (PNG, OGG, WAV) in `Content/`.

## Adding a new sample

1. Create `samples/SampleName/` with `src/` and optionally `Content/`.
2. Add `cna_add_sample(sample_name SOURCES src/Program.cpp ...)` in `CMakeLists.txt`.
3. Uncomment the matching `add_subdirectory(samples/SampleName)` in root `CMakeLists.txt`.
4. Implement `GetTypeName()` in every `Game` subclass.
5. Use `getXxxProperty()` / `setXxxProperty()` for all property access — no direct
   member access.
6. See `DEFERRED.md` for known gaps that may require CNA changes.

## Related projects

- `../cna` — CNA framework (build dependency)
- `../sharp-runtime` — .NET BCL types (transitive dependency via CNA)
- FNA reference: `/rv/data/library/github.com/FNA-XNA/FNA`
