# Missing / Differences from XNA 4.0 original

**Status: not yet ported — SINGLE-blocked as of 2026-07-06 (was double-blocked).**
Blocker 2 below (networking) was accurate when first written this same session, but
a live check of `../cna` right after found `NetworkSession`/`GamerServices`/
`NetworkSessionType::SystemLink`/LAN discovery (`ENetDiscoveryService`) already
fully implemented, merged via `cna`'s `feature/net` branch on 2026-07-04 — two days
before this file's first version claimed the gap. DEFERRED.md item #17 is marked
resolved. Blocker 1 (shaders) still stands — see below. This directory only holds
this write-up (plus a verbatim copy of `readme.htm`); no `src/`/`CMakeLists.txt`
exist yet — see CLAUDE.md's "Adding a new sample" steps for what's still needed once
Blocker 1 is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/NetRumble_4_0/NetRumble/`.

## Blocker 1: Custom HLSL `.fx` shaders (bloom post-process + clouds background)
**XNA behaviour:** NetRumble applies the same multi-pass Gaussian bloom
post-process pipeline already documented in `samples/BloomSample/missing.md`
(`BloomExtract.fx` → `GaussianBlur.fx` → `BloomCombine.fx`, driven by a
`BloomComponent`/`DrawableGameComponent`), plus its own additional `Clouds.fx` for
the scrolling space-background effect. Confirmed present:
```
Content/Effects/Clouds.fx
Content/BloomPostprocess/Effects/GaussianBlur.fx
Content/BloomPostprocess/Effects/BloomCombine.fx
Content/BloomPostprocess/Effects/BloomExtract.fx
```
(4 shaders total — 3 identical in role to BloomSample's bloom trio, plus `Clouds.fx`.)

**CNA port behaviour:** N/A yet (not ported). Same gap as BloomSample: HLSL `.fx`
must be rewritten as GLSL + a `.shader.json` descriptor before `Content.Load<Effect>()`
can use it.

**Root cause:** HLSL→GLSL shader conversion has not been done for any of these 4
effects.

**Tracked in:** DEFERRED.md item #11.

## Blocker 2: Multiplayer networking — ✅ RESOLVED 2026-07-06
**XNA behaviour:** NetRumble's lobby/session flow (`MainMenuScreen.cs`,
`LobbyScreen.cs`, `SearchResultsScreen.cs`, `GameplayScreen.cs`, `World.cs`,
`Ship.cs`, `NetRumbleGame.cs`) is built on
`Microsoft.Xna.Framework.GamerServices`/`Net`'s `NetworkSession` API — creating and
finding sessions via `NetworkSessionType.SystemLink` (LAN) and
`NetworkSessionType.PlayerMatch`/`Ranked` (matchmaking).

**CNA port behaviour:** `NetworkSession`, `NetworkSessionType` (including
`SystemLink`), `GamerServicesComponent`, `AvailableNetworkSessionCollection`, and
real LAN discovery (`CNA::Internal::Net::ENetDiscoveryService`) are all now
implemented (`include/Microsoft/Xna/Framework/Net/*.hpp`, `NetworkSession.cpp` at
836 lines) — merged via `cna`'s `feature/net` branch on 2026-07-04.

**Root cause (historical):** was a real, unimplemented engine feature; now resolved.

**Tracked in:** DEFERRED.md item #17 (marked resolved).

## Summary
NetRumble is now **single-blocked** — only DEFERRED.md item #11 (shader conversion,
4 `.fx` files: `Clouds.fx` + the 3-shader bloom trio) remains. The networking half
is fully unblocked; a port attempt could reasonably stub/skip the bloom
post-process initially (toggle-off-able at runtime per the original's own `B` key)
and land the multiplayer gameplay first, adding bloom once item #11 lands for any
sample.

**Tracked in:** DEFERRED.md item #11.
