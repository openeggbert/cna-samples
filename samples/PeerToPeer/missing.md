# Missing / Differences from XNA 4.0 original

**Status: not yet ported.** This directory only holds this write-up (plus a verbatim
copy of `PeerToPeer.htm`) so the CNA-side blocker is documented where a future porting
session will look. No `src/`/`CMakeLists.txt` exist yet — see CLAUDE.md's "Adding a
new sample" steps for what's still needed once the gap below is fixed.

Source: `/rv/tmp/XNAGameStudio/Samples/PeerToPeerSample_4_0/PeerToPeer/PeerToPeerGame.cs`
(confirmed: `using Microsoft.Xna.Framework.GamerServices;` and
`using Microsoft.Xna.Framework.Net;` at lines 13/16,
`networkSession = NetworkSession.Create(NetworkSessionType.SystemLink, maxLocalGamers,
maxGamers);` at line 142-143).

## Blocker: `Microsoft.Xna.Framework.Net.NetworkSession` (full peer-to-peer topology) is not implemented

**XNA behaviour:** Uses the same `NetworkSession.Create`/`Find`
(`NetworkSessionType.SystemLink`) session APIs as ClientServerSample and
NetworkPrediction, but with **no host authority at all** — every machine fully
simulates and owns its own tank locally (`UpdateLocalGamer()`, lines 253-270:
`localTank.Update()` runs the identical physics on every peer) and simply broadcasts
its own resulting position/rotation to everyone in the session every single frame via
`gamer.SendData(...)` — there is no `IsHost`/server-only branch anywhere in
`UpdateNetworkSession()` (lines 226-247), unlike ClientServerSample's
`if (networkSession.IsHost) UpdateServer();`. Remote tanks are simply overwritten
wholesale from whatever the last packet said (`ReadIncomingPackets()`, lines
276-298) — no throttled send rate, no prediction, no smoothing, no simulated
latency/packet loss (contrast NetworkPrediction, which adds all of that on top of
this same basic session/packet plumbing). This is the simplest possible *topology*
of the three, but architecturally the most different: state authority is fully
distributed rather than centralized on one machine.

**CNA port behaviour:** N/A yet (not ported).

**Root cause:** A missing engine feature, same underlying gap as ClientServerSample
and NetworkPrediction: `NetworkSession`, `NetworkSessionType`,
`GamerServicesComponent`, `LocalNetworkGamer`/`NetworkGamer`, and `PacketWriter`/
`PacketReader` do not exist anywhere in `cna/include` or `cna/src`. The *technique*
this sample demonstrates — full-broadcast peer-to-peer topology with no central
authority — is generic networking, not Xbox-Live-account-specific, so a from-scratch
CNA-native `NetworkSession`-alike could plausibly support it once the base
create/find/join and send/receive plumbing exists.

**What's needed in `cna`:** The same base session/transport layer ClientServerSample
needs (`NetworkSession.Create`/`Find`/`Join`, `GamerServicesComponent`,
`LocalNetworkGamer`/`NetworkGamer.SendData`/`ReceiveData`/`IsDataAvailable`, packet
reader/writer). No extra authority/prediction machinery is required beyond that —
this sample is topologically the simplest of the three to reason about, even though
it is not recommended as the *first* prototype target (ClientServerSample's
single-authority model is easier to verify correctness against).

**Tracked in:** DEFERRED.md item #17
