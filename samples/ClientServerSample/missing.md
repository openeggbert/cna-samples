# Missing / Differences from XNA 4.0 original

**Status: UNBLOCKED, not yet ported — corrected 2026-07-06.** The blocker described
below (no CNA `NetworkSession`/`GamerServices` support) was accurate when first
written this same session, but a live check of `../cna` right after found it had
already been resolved: a full, real implementation exists (`include/Microsoft/Xna/
Framework/Net/{NetworkSession,NetworkSessionType,NetworkSessionProperties,
AvailableNetworkSession,AvailableNetworkSessionCollection,...}.hpp`,
`NetworkSession.cpp` at 836 lines, `GamerServices/GamerServicesComponent`, real LAN
discovery via `CNA::Internal::Net::ENetDiscoveryService`, and `NetworkSessionType`
including `SystemLink` — exactly what this sample uses), merged via `cna`'s
`feature/net` branch on 2026-07-04 — two days before this file's first version
claimed the gap. DEFERRED.md item #17 is marked resolved. No CNA networking gap
remains; this is now a normal, straightforward porting candidate — see CLAUDE.md's
"Adding a new sample" steps. (Kept the original blocker write-up below for
reference/history.)

Source: `/rv/tmp/XNAGameStudio/Samples/ClientServerSample_4_0/ClientServer/ClientServerGame.cs`
(confirmed: `using Microsoft.Xna.Framework.GamerServices;` and
`using Microsoft.Xna.Framework.Net;` at lines 13/16,
`networkSession = NetworkSession.Create(NetworkSessionType.SystemLink, maxLocalGamers,
maxGamers);` at line 142-143).

## Blocker: `Microsoft.Xna.Framework.Net.NetworkSession` (client-server authority) is not implemented

**XNA behaviour:** This is the simplest of the three GamerServices networking samples
in this batch — a **single authoritative server**. `GamerServicesComponent` is added
in the constructor (line 63) and a LAN session is created/found via
`NetworkSession.Create`/`NetworkSession.Find(NetworkSessionType.SystemLink, ...)`
(lines 142, 165). Every machine reads its own local tank's input and, *if it is not
the host*, sends only that input to the server (`UpdateLocalGamer()`, lines 266-287:
"Only send if we are not the server... There is no point sending packets to
ourselves"). Only the host runs `UpdateServer()` (lines 296-318), which calls
`tank.Update()` for **every** gamer's tank (local and remote alike) and broadcasts the
combined authoritative position/rotation state for the whole session in one packet
via `LocalNetworkGamer.SendData`. Clients never simulate remote tanks themselves —
`ClientReadGameStateFromServer()` (lines 353-390) just overwrites `tank.Position`/
`TankRotation`/`TurretRotation` straight from the server's packet. There is no
prediction, smoothing, or simulated latency/packet-loss anywhere in this sample —
that is what NetworkPrediction adds on top of the same basic session APIs.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** A missing engine feature. `NetworkSession`, `NetworkSessionType`,
`GamerServicesComponent`, `AvailableNetworkSessionCollection`, `LocalNetworkGamer`/
`NetworkGamer`, and `PacketWriter`/`PacketReader` do not exist anywhere in
`cna/include` or `cna/src` (confirmed by grep — zero matches). This is XNA's
session-discovery-and-lockstep layer built on Xbox LIVE/Games for Windows Live in the
original, but the *technique* this sample demonstrates — plain client-server
authority over a LAN session — is generic networking, not Xbox-Live-account-specific.
A from-scratch CNA-native `NetworkSession`-alike (e.g. UDP/TCP sockets with LAN
broadcast discovery, mirroring the public `NetworkSession` surface) could plausibly
stand in for it without needing any real Xbox Live/GfWL service.

**What's needed in `cna`:** A session/transport layer exposing at minimum
`NetworkSession.Create`/`Find`/`Join`, `GamerServicesComponent`, `LocalNetworkGamer`/
`NetworkGamer` with `SendData`/`ReceiveData`/`IsDataAvailable`, and packet
reader/writer helpers. Recommended as the **first** of the four networking samples to
prototype against, since it needs no prediction/smoothing/lockstep logic on top —
just create/find/join plus one-way authoritative state broadcast.

**Tracked in:** DEFERRED.md item #17
