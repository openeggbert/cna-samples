# Ignored Samples

This file lists every sample from the XNA Game Studio archive
(`/rv/tmp/XNAGameStudio/Samples`, 153 subdirectories total — see `PLAN.md`'s Sample
Count Summary) that **will never get a directory under `samples/`**, no matter how
capable CNA eventually becomes. This is different from a `⚠️ Deferred` entry in
`PLAN.md`/`DEFERRED.md`: a deferred sample gets a placeholder directory
(`<Name>.htm` + `missing.md`) today and real source once its CNA gap is fixed. An
ignored sample gets neither, because the reason it's excluded is not a CNA capability
gap — it's one of:

- **Not an XNA 4.0 sample at all** (older XNA 2.0/3.0/3.1 archive, Silverlight/WP7
  native code, art-only asset pack, third-party community kit, etc.) — outside this
  repo's stated scope of porting the XNA Game Studio **4.0** collection.
- **Not a runnable `Game`** (a WinForms design-time tool, a content-pipeline
  extension with no executable, a font/asset/documentation-only directory).
- **Redundant** — a "training kit" exercise variant of a game this repo has already
  ported in its normal form (e.g. `HoneycombRushTrainingKit` vs. the already-ported
  `HoneycombRush`), or a duplicate phone/VB/Mango variant of a sample already listed
  elsewhere in `PLAN.md`.
- **Tied to a platform/service CNA (a desktop C++ game framework) will never
  provide**: WinForms hosting, Windows Phone OS APIs (push notifications, phone
  media library, phone GPS), the Xbox LIVE Avatar body/animation content system, or
  a third-party web API (Bing Maps).

If a sample's exclusion reason ever stops applying (rare, but e.g. if CNA grew a
WinForms-less equivalent tool), it should be *removed from this file* and re-added to
`PLAN.md` as a normal Todo/Deferred entry rather than silently left out.

---

## Not XNA 4.0 (out of this repo's stated scope)

### XNA 2.0 / 3.0 / 3.1 archive samples
Older XNA API versions; this repo ports the XNA Game Studio **4.0** collection only.
| Name | Source Directory | Reason |
|---|---|---|
| BasicEffectShader | `BasicEffectShader_ARCHIVE_2_0` | XNA 2.0, not 4.0 |
| Catapult | `Catapult_ARCHIVE_2_0` | XNA 2.0; superseded by the already-ported `CatapultWars` (#067, XNA 4.0) |
| MaterialsAndLights | `MaterialsAndLights_ARCHIVE_2_0` | XNA 2.0, not 4.0 |
| Minjie | `Minjie_ARCHIVE_2_0` | XNA 2.0, not 4.0 |
| MultipassLighting | `MultipassLighting_ARCHIVE_2_0` | XNA 2.0, not 4.0 |
| Pickture | `Pickture_ARCHIVE_2_0` | XNA 2.0, not 4.0 |
| RobotGame | `RobotGame_ARCHIVE_2_0` | XNA 2.0, not 4.0 |
| SpriteBatchShader | `SpriteBatchShader_ARCHIVE_2_0` | XNA 2.0, not 4.0 |
| VectorRumble | `VectorRumble_ARCHIVE_2_0` | XNA 2.0, not 4.0 |
| SpaceShooter | `SpaceShooter_ARCHIVE_3_0` | XNA 3.0, not 4.0 |
| TiledSprites | `TiledSpritesSample_ARCHIVE_3_1` | XNA 3.1, not 4.0 |
| RedistributableTTFs | `RedistributableTTFs_ARCHIVE_3_1` | XNA 3.1; font files only, no sample code |

### Avatar asset/rig packs (art only, no C# code)
| Name | Source Directory | Reason |
|---|---|---|
| AvatarAnimPack (BIN) | `AvatarAnimPack_4_0_BIN` | Art/animation data, no C# game code |
| AvatarAnimPack (FBX) | `AvatarAnimPack_4_0_FBX` | Art/animation data, no C# game code |
| AvatarAnimPack (Maya) | `AvatarAnimPack_4_0_Maya` | Art/animation data, no C# game code |
| AvatarAnimPack (Mod Tool) | `AvatarAnimPack_4_0_Mod_Tool` | Art/animation data, no C# game code |
| AvatarRig (3ds Max 2010) | `AvatarRig_4_0_Max_2010` | DCC rigging data, no C# game code |
| AvatarRig (Maya 2009) | `AvatarRig_4_0_Maya_2009` | DCC rigging data, no C# game code |
| AvatarRig (SoftImage Mod Tool 7.5) | `AvatarRig_4_0_SoftImage_Mod_Tool7_5` | DCC rigging data, no C# game code |

### Phone / Mango platform variants (duplicates of a desktop sample listed elsewhere)
| Name | Source Directory | Reason |
|---|---|---|
| GSMSample (Mango) | `GSMSample_4_0_Mango` | Duplicate of already-ported `GameStateManagement` (#072) |
| GSMSample (Mango VB) | `GSMSample_4_0_Mango_VB` | Duplicate of #072, and a VB dup on top |
| GSMSample (Phone) | `GSMSample_4_0_PHONE` | Duplicate of already-ported `GameStateManagement` (#072) |
| ModelViewerDemo (Mango) | `ModelViewerDemo_4_0_Mango` | Phone-only, no desktop equivalent; WP7-specific model viewer UI |
| PaddleBattle (Mango) | `PaddleBattle_4_0_Mango` | Phone-only, no desktop equivalent |
| PaddleBattle (Mango VB) | `PaddleBattle_4_0_Mango_VB` | Phone-only, and a VB dup on top |
| RolePlayingGame (Phone) | `RolePlayingGame_4_0_Phone` | Duplicate of already-ported `RolePlayingGame` (#070) |

### VB language duplicates
| Name | Source Directory | Reason |
|---|---|---|
| CardsStarterKit (VB) | `CardsStarterKit_4_0_VB` | Duplicate of already-ported `CardsStarterKit` (#069), Visual Basic instead of C# |

### Silverlight / Windows Phone 7 native code (not XNA)
| Name | Source Directory | Reason |
|---|---|---|
| CustomIndeterminateProgressBar | `CustomIndeterminateProgressBarSample` | Silverlight control, not XNA |
| NonLinear WP SL Navigation | `NonLinear-WP-SLApp-Navigation-Service` | Silverlight app, not XNA |
| PushRecipe WP7 | `PushRecipe_WP7_SL` | Silverlight + WP7 push plumbing, not XNA |
| SilverlightMicrophone | `SilverlightMicrophoneSample` | Silverlight, not XNA (see MicrophoneEcho below for the XNA microphone sample, which is a future candidate) |
| TombstoningSample | `TombstoningSample` | Silverlight/WP7 app lifecycle demo, not XNA |
| LevelStarterKit | `LevelStarterKit` | WP7 Silverlight app (`App.xaml`/`MainPage.xaml`), not an XNA `Game` |

### Image / resource-only directories (no code)
| Name | Source Directory | Reason |
|---|---|---|
| ButtonImages | `ButtonImages` | Image assets only, used by other samples |
| ControllerImages | `ControllerImages` | Image assets only, used by other samples |
| LobbyChatImages | `LobbyChatImages` | Image assets only, used by other samples |

### Third-party / community kits (not official Microsoft samples)
| Name | Source Directory | Reason |
|---|---|---|
| Riemers Tutorials | `Riemers` | Third-party XNA tutorial series, not an official sample |
| XNA 4 Racing Game Kit | `XNA-4-Racing-Game-Kit-master` | Community game kit, not an official sample |
| Movipa | `Movipa` | Third-party video playback sample, not an official sample |

### Unversioned / incomplete starter kits
| Name | Source Directory | Reason |
|---|---|---|
| UnitConverterStarterKit | `UnitConverterStarterKit` | No version tag; directory contains only a license file and an empty stub subfolder — no real sample content |

### Misc / non-code
| Name | Source Directory | Reason |
|---|---|---|
| XNA XNB Format | `XNA_XNB_Format` | Binary format documentation, no C# code |
| SoundLab | `SoundLab` | Standalone audio authoring tool, not an XNA game sample |

---

## Not a runnable `Game` (tool / library / content-pipeline-only, even within XNA 4.0)

| # | Name | Source Directory | Reason |
|---|---|---|---|
| 004 | StockEffects | `StockEffectsSample_4_0` | Ships an effect-source + CLI compiler, no runnable `Game` |
| 090 | BitmapFontMaker | `BitmapFontMaker_4_0` | WinForms design-time tool, not a `Game` |
| 092 | ContentManifestExtensions | `ContentManifestExtensions_4_0` | Content-pipeline extension only, no executable |
| 093 | CurveEditor | `CurveEditor_4_0` | WinForms animation-curve editing tool, not a `Game` |
| 097 | MemoryMadnessLab | `MemoryMadnessLab_4_0` | WP7 "Multi-touch Game Development" teaching lab + accompanying Word doc, not a standalone runnable sample |
| 099 | ModelImporterSample | `ModelImporterSample_4_0` | Content-pipeline extension only, no executable |
| 104 | PerformanceUtility | `PerformanceUtility_4_0` | Utility library only, no standalone executable |
| 108 | WinFormsContent | `WinFormsContentSample_4_0` | WinForms host window, not a `Game` |
| 109 | WinFormsGraphics | `WinFormsGraphicsSample_4_0` | WinForms host window, not a `Game` |
| 111 | XnaGraphicsProfileChecker | `XnaGraphicsProfileChecker_4_0` | WinForms diagnostic tool, not a `Game` |

## Redundant "training kit" duplicates of an already-ported sample

| # | Name | Source Directory | Reason |
|---|---|---|---|
| 064 | HoneycombRushTrainingKit | `HoneycombRushTrainingKit_4_0` | Multi-exercise teaching variant (`Sources/Starter`, `Ex1`, `Ex2`) of the already-ported `HoneycombRush` (#063) — not one clean sample, just exercises building toward the same game |
| 068 | CatapultWarsTrainingKit | `CatapultWarsTrainingKit_4_0` | Multi-exercise teaching variant (`Exercises/Ex1`–`Ex6`, `Advanced`) of the already-ported `CatapultWars` (#067) |

## Would only ever demonstrate an empty stub

| # | Name | Source Directory | Reason |
|---|---|---|---|
| 075 | NGSMSample | `NGSMSample_4_0` | ~1900 of its ~2900 lines are Xbox LIVE lobby/session/gamer-profile networking (item 17 in `DEFERRED.md`); even once that exists, the sample's own "Single Player" path is an intentionally empty `GameplayScreen` stub per the original's own documentation ("there is no actual game code included here") — porting it would only ever produce a hollow lobby demo, never real gameplay |

## Tied to a platform/service CNA will never provide

| # | Name | Source Directory | Reason |
|---|---|---|---|
| 085 | AvatarAnimationBlending | `AvatarAnimationBlendingSample_4_0` | Renders the Xbox LIVE Avatar body using Microsoft's proprietary Avatar content system (`AvatarRenderer`/`AvatarAnimation`) — that content was retired with the original Xbox Avatars marketplace and cannot be reproduced. **Reconsider (2026-07-06):** `cna` has since grown an opt-in `AvatarRenderer::EnableRealRenderingEXT` path (`docs/avatar-real-rendering-ext.md`, `tools/avatar_asset_pipeline/convert_avatar.py`) that renders a real GPU-skinned mesh from a MakeHuman+Mixamo asset instead of the real (permanently unavailable) Xbox body — explicitly *not* a reproduction of the original art. Porting this sample onto that substitute body is now technically possible; whether it's worth doing (same class of call as `AccelerometerSample`'s keyboard-tilt scope decision) is a product decision, not settled here — see `PLAN.md`/`NEXT.md` |
| 086 | AvatarMultipleAnimations | `AvatarMultipleAnimationsSample_4_0` | Same Xbox LIVE Avatar system dependency as #085 — same EXT-path reconsideration applies |
| 087 | AvatarShadows | `AvatarShadows_4_0` | Same Xbox LIVE Avatar system dependency as #085 — same EXT-path reconsideration applies |
| 094 | CustomAvatarAnimation | `CustomAvatarAnimation_4_0` | Same Xbox LIVE Avatar system dependency as #085 — same EXT-path reconsideration applies |
| 101 | ObjectPlacementOnAvatar | `ObjectPlacementOnAvatarSample_4_0` | Same Xbox LIVE Avatar system dependency as #085 — same EXT-path reconsideration applies |
| 088 | BingMaps | `BingMaps_4_0` | Requires a Bing Maps Web API subscription key / the (long-deprecated) Bing Maps Silverlight/AJAX control — an external web service, not a CNA framework capability |
| 089 | BingMapsPathFinding | `BingMapsPathFinding_4_0` | Same Bing Maps external-service dependency as #088 |
| 095 | GeolocationSample | `GeolocationSample_4_0` | Real phone GPS hardware; unlike the accelerometer (SDL_Sensor covers that generically, see `AccelerometerSample` below), SDL has no portable geolocation API to build on |
| 096 | InvitesSample | `InvitesSample_4_0` | Xbox LIVE friends/invite/presence system — tied to a real Xbox LIVE account, not just LAN `NetworkSession` discovery (contrast with `ClientServerSample`/`NetworkPrediction`/`PeerToPeer`, which are future candidates — see `PLAN.md`) |
| 105 | PushNotifications | `PushNotificationsSample_4_0` | Windows Phone push notification service; no desktop analog |
| 106 | SavingEmbeddedImages | `SavingEmbeddedImages_4_0` | Windows Phone media library API; no desktop analog |
| 110 | WP7MusicManagement | `WP7MusicManagement_4_0` | Windows Phone 7 media-library management API; no desktop analog (contrast with the already-ported `SoundAndMusic`/`Audio3D`, which use the portable `SoundEffect`/`Song` APIs) |

---

## Total

67 samples ignored (of 153 total XNA sample directories). See `PLAN.md` for the
83 (soon 84, with `Orientation` relocated in) XNA 4.0 desktop samples and 27 phone/
hardware/networking `_4_0` samples this repo tracks instead, and `DEFERRED.md` for
the specific CNA capability gap blocking each not-yet-ported one.
