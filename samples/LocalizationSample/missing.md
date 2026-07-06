# Missing / Differences from XNA 4.0 original

## Added: manual language cycling instead of OS locale detection
**XNA behaviour:** The original reads `CultureInfo.CurrentCulture` (the Windows/Xbox OS
locale) once at startup and shows whichever of the five bundled languages (English,
Danish, French, Japanese, Korean) matches, via `Strings.resx`'s satellite-assembly
fallback and a `LoadLocalizedAsset<T>` helper that tries the full culture name, then the
language-only name, then the default asset.
**CNA port behaviour:** `sharp-runtime`'s `System::Globalization::CultureInfo::CurrentCulture`
is a documented stub that always returns the invariant culture (no real OS locale
detection). Since there's nothing to auto-detect, SPACE manually cycles through the same
five languages instead, calling the same `LoadLocalizedAsset<T>` fallback algorithm for
the `Flag` texture on every change (e.g. "Flag.da-DK" fails, "Flag.da" succeeds). The
`Strings.resx` family is ported as a static table (`Strings.hpp`) rather than a generated
resource class, since there is no .NET satellite-assembly / resx-compilation step in C++.
**Root cause:** No OS locale API in sharp-runtime.
**Tracked in:** Not planned — deliberate, documented adaptation.

## LoadLocalizedAsset catches std::runtime_error instead of ContentLoadException
**XNA behaviour:** `LoadLocalizedAsset<T>` catches `ContentLoadException` specifically
around each speculative `Content.Load<T>(localizedAssetName)` call, since that is the
only exception type `ContentManager.Load<T>` ever throws for a missing/unloadable asset.
**CNA port behaviour:** The port's `LoadLocalizedAsset<T>` catches `std::runtime_error`
(the common base class) instead of `ContentLoadException` specifically. This is
necessary because `ContentManager::Load<Graphics::Texture2D>` does not uniformly throw
`ContentLoadException` on failure the way its Doxygen contract states: `Texture2DTypeReader::Read`
forwards straight to `Texture2D(path, device)`, which calls `CNA::Internal::Graphics::ImageLoader::Load`,
which throws a bare `std::runtime_error("Failed to load image: ...")` from SDL_image when
a candidate path (e.g. `"Images/Flag.ja"`) doesn't exist — it never gets wrapped as a
`ContentLoadException`. Catching the shared `std::runtime_error` base (which
`ContentLoadException` also derives from) is required to catch both cases uniformly.
**Root cause:** `ContentManager::Load<Texture2D>`'s missing-file path lets a raw
`std::runtime_error` from the SDL_image-backed `ImageLoader` propagate instead of
wrapping it in `ContentLoadException`, unlike the generic `ContentManager::Load<T>` path.
**Tracked in:** Not filed as a CNA issue yet; harmless here since the broader catch
still isolates only "this asset variant is missing," but a future CNA fix should make
`Load<Texture2D>` wrap loader failures in `ContentLoadException` for parity with the
generic path and with XNA.

## CNA framework fixes made while porting this sample
Porting this sample surfaced two real CNA bugs, both fixed in the `cna` repo (not
worked around here) since they weren't specific to this sample:

1. **`ContentManager::ResolveAssetPath` misresolved culture-suffixed asset names**
   (e.g. `"Flag.en-US"`). It used `std::filesystem::path::has_extension()` to decide
   whether to append a reader extension (`.png`); that heuristic treats *any* trailing
   `.something` as an extension, so `"Flag.en-US"` was mistaken for an already-resolved
   path ending in a `.en-US` "extension" and CNA never tried `"Flag.en-US.png"`. Fixed to
   check literal-path existence first, falling back to trying reader extensions
   otherwise — correct for both ordinary and culture-suffixed names.
2. **`SpriteFont::MeasureString` and `SpriteBatch::DrawString` had no UTF-8 decoding.**
   Both iterated the input `std::string` one raw `char` at a time and cast each byte
   directly to `charcs` (`char16_t`), so any multi-byte UTF-8 character — accented Latin
   (æ, é, à, ...), Japanese, Korean, anything outside ASCII — split into 2-4 bytes that
   each failed the glyph lookup and rendered as `?` placeholders. This is not specific to
   this sample; it affected any CNA text containing non-ASCII characters. Fixed by adding
   `CNA::Internal::DecodeUtf8CodePoint()` (`CNA/Internal/Utf8Decode.hpp`) and using it in
   both places instead of the raw-byte cast. Verified: all five languages (including
   Japanese and Korean) now render correctly; a full rebuild of all 33 cna-samples
   targets and a spot-check of an ASCII-heavy sample (InputReporter) showed no
   regressions.

Both fixes were confirmed with the maintainer before being made (see NEXT.md).

## No known differences beyond the above
The three-line welcome/locale/how-to-change text, the flag display, and the
`LoadLocalizedAsset<T>` fallback algorithm are a faithful port of `LocalizationGame.cs`.
Screen resolution (800x480) matches the original's implicit default.
