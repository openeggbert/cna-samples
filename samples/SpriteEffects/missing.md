# Missing / Differences from XNA 4.0 original

## Custom effect uniforms not applied via SpriteBatch

**XNA behaviour:** `effect.Parameters["OverlayScroll"].SetValue(vec2)`,
`effect.Parameters["LightDirection"].SetValue(vec3)`, and
`effect.Parameters["DisplacementScroll"].SetValue(vec2)` write into the
compiled HLSL program and are read by the fragment shader during each Draw call.

**CNA port behaviour:** `ShaderEffect::SetUniformVec2` / `SetUniformVec3`
write uniforms into the ShaderEffect's internal `IEffectBackend` program.
However, the `EasyGLSpriteBatchBackend::FlushBatch()` compiles a separate copy
of the same GLSL source into its own `customProgram_` and uses that one for
the actual draw — the IEffectBackend program is used only for its `glUseProgram`
side-effect inside `Apply()`. As a result, uniforms set via `SetUniform*` have
no effect on the rendered output. The `OverlayScroll`, `LightDirection`, and
`DisplacementScroll` uniforms remain at their GLSL default of 0.

**Root cause:** CNA `EasyGLSpriteBatchBackend` does not propagate user-set
uniforms to its internal `customProgram_` after compiling it.

**Tracked in:** DEFERRED.md (new item needed — SpriteBatch custom effect
uniform forwarding).

---

## Secondary texture (Textures[1]) not bound during SpriteBatch rendering

**XNA behaviour:** `GraphicsDevice.Textures[1] = overlayTexture` sets the
texture bound to sampler register `s1`. When SpriteBatch draws with a custom
HLSL effect, the GPU reads from both `s0` (sprite texture) and `s1` (overlay).

**CNA port behaviour:** `device.getTexturesProperty()(1, &texture)` stores the
texture reference in `TextureCollection`, but `EasyGLSpriteBatchBackend::FlushBatch()`
never reads the `TextureCollection` to bind secondary textures to additional GL
texture units. Only the sprite's own texture is bound to unit 0.
Samplers for `OverlaySampler`, `NormalSampler`, and `DisplacementSampler` in
the fragment shaders default to unit 0 and therefore sample the sprite texture
instead of the intended overlay/normalmap/displacement texture.

**Effects impacted:**
- **Disappear**: fade speed is derived from the sprite's own red channel instead
  of the waterfall overlay → wrong fade pattern
- **Normalmap**: surface normals are the sprite's RGB values, light direction is
  (0,0,0) → black cat on glacier background
- **Refraction**: displacement is based on the sprite's own colors at a scaled
  coordinate → mild warp that is unrelated to the waterfall texture

**Root cause:** CNA `EasyGLSpriteBatchBackend` does not iterate
`GraphicsDevice.getTexturesProperty()` to bind additional GL texture units.

**Tracked in:** DEFERRED.md (new item needed — SpriteBatch multi-texture
support for custom effects).

---

## Desaturate effect works correctly

**XNA behaviour / CNA port behaviour:** Both are identical. The saturation
level is encoded in `Color.a` (the per-vertex alpha passed to `SpriteBatch.Draw`),
which is a vertex attribute (`aColor.a` at layout location 2) that SpriteBatch
forwards correctly to the custom fragment shader. No secondary texture or
extra uniform is needed — the single-texture desaturate GLSL shader produces
output identical to the HLSL original.

---

## cat_normalmap texture replaced by cat_depth

**XNA behaviour:** The Content Pipeline runs `NormalMapProcessor.cs` on
`cat_depth.jpg` to produce a proper three-component normal map (`cat_normalmap`).

**CNA port behaviour:** `cat_depth.jpg` (the greyscale source image) is
copied verbatim to `Content/cat_normalmap.jpg`. It is used directly as the
normal-map texture, resulting in incorrect lighting computations even when
the normalmap effect secondary-texture binding issue (above) is resolved.

**Root cause:** CNA has no Content Pipeline. Conversion must be done offline
or replicated at runtime.

**Tracked in:** Not in DEFERRED.md; low priority since the normalmap effect
has a deeper CNA blocker (see above).

---

## cat texture has no alpha channel

**XNA behaviour:** `TexturePlusAlphaProcessor.cs` combines `cat.jpg` (RGB)
with `cat_alpha.jpg` (greyscale alpha mask) to produce a `cat` asset with
proper per-pixel transparency.

**CNA port behaviour:** `Content.Load<Texture2D>("cat")` loads `cat.jpg`
directly. The cat is rendered as a solid rectangle with no transparency.
The Disappear and Refraction effects are affected (the fade/warp applies to
the whole rectangle instead of just the cat silhouette).

**Root cause:** CNA has no Content Pipeline.

**Tracked in:** Not in DEFERRED.md; minor visual difference.
