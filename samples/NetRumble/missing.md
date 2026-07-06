# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `readme.htm`) so the CNA-side blockers are documented in the same place a
future porting session will look. No `src/`/`CMakeLists.txt` exist yet — see
CLAUDE.md's "Adding a new sample" steps for what's still needed once the CNA gaps
below are fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/NetRumble_4_0/NetRumble/`. This sample is
**double-blocked** — it needs two independent, unrelated CNA features before it can
be ported at all.

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

## Blocker 2: Multiplayer networking (`GamerServices.NetworkSession`)
**XNA behaviour:** NetRumble's lobby/session flow (`MainMenuScreen.cs`,
`LobbyScreen.cs`, `SearchResultsScreen.cs`, `GameplayScreen.cs`, `World.cs`,
`Ship.cs`, `NetRumbleGame.cs`) is built on
`Microsoft.Xna.Framework.GamerServices`/`Net`'s `NetworkSession` API — creating and
finding sessions via `NetworkSessionType.SystemLink` (LAN) and
`NetworkSessionType.PlayerMatch`/`Ranked` (matchmaking), e.g.:
```csharp
// MainMenuScreen.cs
CreateSession(NetworkSessionType.SystemLink);
FindSession(NetworkSessionType.SystemLink);
CreateSession(NetworkSessionType.PlayerMatch);
FindSession(NetworkSessionType.PlayerMatch);
```
Confirmed via grep — `NetworkSession`/`GamerServicesComponent`/`NetworkSessionType`
usage appears across `NetRumbleGame.cs`, `MainMenuScreen.cs`, `LobbyScreen.cs`,
`GameplayScreen.cs`, `SearchResultsScreen.cs`, `World.cs`, and `Ship.cs`.

**CNA port behaviour:** N/A yet (not ported). `NetworkSession`, `NetworkSessionType`,
`GamerServicesComponent`, `AvailableNetworkSessionCollection`, and related types do
not exist anywhere in `cna/include` or `cna/src` (zero matches by grep).

**Root cause:** CNA has no networking/session layer at all yet — this is
engine-level feature work (a from-scratch LAN session/transport layer standing in
for XNA's Xbox LIVE/GfWL-backed `NetworkSession`), not an asset-conversion gap.
DEFERRED.md item #17 already names NetRumble by name as blocked by this, alongside
its shader-pipeline blocker (see Blocker 1 above) — "double-blocked".

**Tracked in:** DEFERRED.md item #17.

## Summary
NetRumble needs **both** DEFERRED.md item #11 (shader conversion, 4 `.fx` files) and
item #17 (a `NetworkSession`-alike networking layer) resolved before it can be
ported — neither alone is sufficient. Per DEFERRED.md item #17's recommendation,
whoever picks up the networking layer should prototype against the simpler
ClientServerSample first (single authoritative server, no prediction/lockstep)
before attempting NetRumble's peer topology.

**Tracked in:** DEFERRED.md items #11 and #17.
