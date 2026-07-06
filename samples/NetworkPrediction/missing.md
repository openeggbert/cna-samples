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

Source: `/rv/tmp/XNAGameStudio/Samples/NetworkPredictionSample_4_0/NetworkPrediction/NetworkPredictionGame.cs`
and `Tank.cs` (confirmed: `using Microsoft.Xna.Framework.GamerServices;` and
`using Microsoft.Xna.Framework.Net;` at lines 13/16,
`networkSession = NetworkSession.Create(NetworkSessionType.SystemLink, maxLocalGamers,
maxGamers);` at line 178-179).

## Blocker: `Microsoft.Xna.Framework.Net.NetworkSession` (prediction/smoothing under latency) is not implemented

**XNA behaviour:** Builds on the same `NetworkSession.Create`/`Find`
(`NetworkSessionType.SystemLink`) session APIs as ClientServerSample and PeerToPeer,
but its whole point is demonstrating **dead-reckoning prediction and interpolation
smoothing to hide network latency and a low packet send rate**, not a particular
session topology. Each gamer fully updates its own tank locally every frame
(`Tank.UpdateLocal()`), but only *sends* its state periodically —
`framesBetweenPackets` throttles the send rate to as low as 1 packet every 6 frames
(`UpdateNetworkSession()`, lines 262-314) to model realistic bandwidth constraints.
For remote tanks, `Tank.UpdateRemote(framesBetweenPackets, enablePrediction)` and
`Tank.ReadNetworkPacket(..., enablePrediction, enableSmoothing)` (`Tank.cs:160-297`)
extrapolate position forward from the last known packet (prediction) and blend
smoothly into newly-arrived authoritative state rather than snapping
(`ApplySmoothing()`/`ApplyPrediction()`, `Tank.cs:205-297`) — using a `RollingAverage`
(`RollingAverage.cs`) of `sender.RoundtripTime` plus `NetworkSession.SimulatedLatency`
to estimate how stale each packet is. The sample also exposes a live-tunable
`NetworkQuality` enum (Typical/Poor/Perfect — `NetworkPredictionGame.cs:65-70`) that
sets `NetworkSession.SimulatedLatency`/`SimulatedPacketLoss` and replicates the
current settings to all clients via `NetworkSession.SessionProperties`
(`UpdateOptions()`, lines 386-451), plus on-screen toggles for prediction/smoothing
(X/Y buttons) and send rate (B button) so the effect of each technique can be seen
directly. None of this — throttled send rate, prediction, smoothing, simulated
latency/packet loss, `SessionProperties` replication — exists in ClientServerSample
or PeerToPeer.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** A missing engine feature, same underlying gap as ClientServerSample
and PeerToPeer: `NetworkSession` and friends (plus, specifically for this sample,
`NetworkSession.SimulatedLatency`/`SimulatedPacketLoss`/`SessionProperties` and
`NetworkGamer.RoundtripTime`) do not exist anywhere in `cna/include` or `cna/src`.
The *technique* — client-side prediction and remote-entity smoothing over a
lockstep-free UDP-style transport — is generic real-time-networking practice, not
Xbox-Live-account-specific, so it's a plausible feature to build once a base
`NetworkSession`-alike exists.

**What's needed in `cna`:** Everything ClientServerSample needs, plus latency/packet-
loss simulation knobs and a replicated `SessionProperties`-style key/value store.
Recommended to prototype *after* ClientServerSample (the simplest of the four), since
prediction/smoothing logic is naturally layered on top of a working create/find/join
plus send/receive session, not a prerequisite for it.

**Tracked in:** DEFERRED.md item #17
