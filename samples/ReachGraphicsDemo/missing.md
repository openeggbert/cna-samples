# Missing / Differences from XNA 4.0 original

## Not implemented — blocked by multiple CNA gaps

This sample is **not yet ported**. It is tracked as `⚠️ Deferred` in `PLAN.md`.

---

## SpriteFont — menu system and all UI text

**XNA behaviour:** The title menu, demo selection menu, and all on-screen
labels use two SpriteFonts (`font`, `bigfont`) rendered via `DrawString`.
`MenuComponent` and `MenuEntry` are built entirely around SpriteFont layout.

**CNA port behaviour:** Not implemented.

**Root cause:** CNA has no SpriteFont support yet.

**Tracked in:** DEFERRED.md (SpriteFont / DrawString).

---

## Model loading — four of six demo scenes

**XNA behaviour:** Four demo scenes load 3D models via `Content.Load<Model>`:
- `BasicDemo`: `"grid"` (BasicEffect lighting demo)
- `AlphaDemo`: `"grid"` (alpha transparency demo)
- `DualDemo`: `"model"` (DualTextureEffect demo)
- `EnvmapDemo`: `"saucer"` (EnvironmentMapEffect demo)

**CNA port behaviour:** Not implemented.

**Root cause:** CNA has no Model (glTF) loading yet.

**Tracked in:** DEFERRED.md (Model loading).

---

## Skeletal animation — SkinnedDemo

**XNA behaviour:** `SkinnedDemo` loads an animated humanoid model (`"dude"`)
with `SkinningData` (bone transforms, animation clips) produced by
`SkinnedModelProcessor`. `AnimationPlayer` drives per-frame bone matrix
updates. Uses custom `SkinnedEffect` HLSL shader.

**CNA port behaviour:** Not implemented.

**Root cause:** CNA has no skeletal animation system or SkinnedEffect yet.

**Tracked in:** DEFERRED.md (SkinnedEffect / animation).

---

## Custom content pipeline types — Sky, SkinnedModel, EnvironmentMappedModel

**XNA behaviour:** The `ContentPipelineExtension` project provides custom
XNA Content Pipeline processors:
- `SkyProcessor` → `Sky` runtime type (cube-map skybox)
- `SkinnedModelProcessor` → `Model` with attached `SkinningData`
- `EnvironmentMappedMaterialProcessor` / `EnvironmentMappedModelProcessor`
- `DualTextureProcessor` / `TexturePlusAlphaProcessor`

These produce XNB assets consumed at runtime by the demo scenes.

**CNA port behaviour:** Not implemented. CNA has no Content Pipeline extension
mechanism and XNB is never supported.

**Root cause:** Requires open-format asset equivalents (glTF + cube-map PNG)
and CNA runtime support for the corresponding effect types.

**Tracked in:** DEFERRED.md (Model loading, SkinnedEffect).

---

## EnvironmentMapEffect and DualTextureEffect

**XNA behaviour:** `EnvmapDemo` uses `EnvironmentMapEffect`; `DualDemo` uses
`DualTextureEffect` — both are XNA built-in effects.

**CNA port behaviour:** Not implemented.

**Root cause:** CNA does not yet expose `EnvironmentMapEffect` or
`DualTextureEffect` as public API.

**Tracked in:** DEFERRED.md.

---

## ParticleDemo — potentially portable once SpriteFont is available

**XNA behaviour:** `ParticleDemo` uses only a single `Texture2D` (`"cat"`)
and 2D sprite rendering. It does not load any Model or use skinning.

**CNA port behaviour:** Not implemented. `ParticleDemo` cannot be extracted
in isolation because the menu system (blocked by SpriteFont) is required to
navigate to it at runtime.

**Root cause:** SpriteFont (menu system dependency).

**Tracked in:** DEFERRED.md (SpriteFont / DrawString).
