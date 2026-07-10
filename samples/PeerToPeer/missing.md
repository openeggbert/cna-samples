# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-10.** Builds with 0 warnings (confirmed via two separate
from-scratch rebuilds — one before, one after removing this session's temporary
debug-auto-trigger code). Ran under `SDL_VIDEODRIVER=x11` for 23-30+ seconds across
three separate runs with no crash.

Source: `/rv/tmp/XNAGameStudio/Samples/PeerToPeerSample_4_0/PeerToPeer/
{PeerToPeerGame.cs, Tank.cs}`. This is the third and final sample in this repo's
`NetworkSession`/`GamerServicesComponent` family (after ClientServerSample #091 and
NetworkPrediction #100), and the first with a genuinely different network *topology*:
full peer-to-peer, with **no host authority over simulation at all** — every machine
fully simulates its own locally-controlled tanks and broadcasts the result to
everyone, rather than one machine (ClientServerSample) or a prediction/smoothing
layer on top of that same split (NetworkPrediction). Read
`samples/ClientServerSample/src/*.hpp`, `samples/NetworkPrediction/src/*.hpp`, and
both of their `missing.md` files in full before starting, per this task's brief.

## No new networking workarounds needed — the "zero workarounds" hypothesis holds a third time

**XNA behaviour:** `PeerToPeerGame`'s constructor adds a real `GamerServicesComponent`
(`Components.Add(new GamerServicesComponent(this));`) exactly like both prior samples.
`UpdateNetworkSession()` has **no** `if (networkSession.IsHost)` branch anywhere — every
local gamer runs `UpdateLocalGamer()` (full physics simulation) every frame regardless
of host/client status, then broadcasts its own tank's position/rotation/turret-rotation
via `gamer.SendData(packetWriter, SendDataOptions.InOrder)` with **no explicit
recipient** (a real full broadcast to everyone in the session, not a client→host or
host→everyone send like the other two samples). `gamer.IsHost` is read only for
`DrawNetworkSession()`'s "(host)" cosmetic label — it plays no role in who simulates
what.

**CNA port behaviour:** Ported line-for-line with no workaround of any kind. The
constructor constructs `gamerServices_ = std::make_unique<GamerServicesComponent>(*this);
getComponentsProperty().Add(gamerServices_.get());` directly, matching the C# original
exactly (same as ClientServerSample/NetworkPrediction, after their own DEFERRED.md
items #19/#20/#21 fixes). `HookSessionEvents()` needs no manual `Update()` call
afterward — `GamerJoined` replays synchronously on subscription, same as the other two
samples. `networkSession_->getIsHostProperty()`/`gamer->getIsHostProperty()` are used
directly with no locally-tracked `bool`. `UpdateLocalGamer()`/`ReadIncomingPackets()`
are direct ports with no host/client branch at all (there is none in the C# original
either) — this is architecturally the *simplest* of the three samples' `Update()` loop
shapes, even though the peer-to-peer topology it demonstrates is conceptually the most
different.

**Confirmed live** via this repo's established temporary debug-auto-trigger pattern
(`CreateSession()` + `helpTimer_ = 10.0f` forced on frame 30, reverted before commit,
re-verified with a clean from-scratch rebuild afterward — `xdotool getactivewindow`
showed a different real user window (`0x400003`, decimal `4194307`) held focus
throughout, this repo's known shared-desktop `xdotool` caveat, so no synthetic
keypresses were sent): `NetworkSession::Create()` completes with no hang;
`GamerJoinedEventHandler` fires synchronously (a fully textured tank labeled
`"Stub Gamer (host)"` renders on the very first frame after the trigger, with no
manual `Update()` call anywhere); the session correctly reports itself as host (the
"(host)" label renders); the F1 help overlay renders correctly on top and correctly
disappears after its 10-second timer (confirmed via two screenshots ~6 seconds apart:
first shows both tank and help overlay, second shows only the tank, overlay gone). No
crash over 30+ seconds of observation.

## `SessionProperties` (DEFERRED.md item #27) never comes up — this sample doesn't use it

**XNA behaviour:** Unlike NetworkPrediction (which relies on `NetworkSession.
SessionProperties` for host→client replication of 4 network-quality/prediction
settings — the reason DEFERRED.md item #27 was filed), `PeerToPeerGame.cs` never
references `SessionProperties` anywhere. It has no host-authoritative game options at
all: the only broadcast state is each tank's own position/rotation, sent directly by
whichever machine controls it.

**CNA port behaviour:** No workaround needed, and none applied — this sample simply
never touches the gap. Confirms this task's brief's own expectation ("this sample
might expose something client/server-only samples didn't, or it might not — say so
clearly either way"): for this specific gap, it doesn't. `Tank.hpp` itself needed no
`PacketKind`-byte disambiguation (unlike `NetworkPredictionGame.hpp`'s
`SendOptionsPacket()`/`PacketKind` workaround) since every packet on the wire is
always the same shape (`Position`, `TankRotation`, `TurretRotation` — nothing else is
ever sent).

**Tracked in:** N/A — no new gap found, item #27 unaffected.

## `Tank.cs` confirmed byte-identical to ClientServerSample's own `Tank.cs`

**XNA behaviour:** Same tank-movement physics (turn rate, speed, friction) and the
same texture/font assets (confirmed via `diff`/`md5sum`: `Tank.cs` differs from
ClientServerSample's own `Tank.cs` only in the surrounding C# namespace; `Tank.tga`,
`Turret.tga`, and `Font.spritefont` are byte-identical, same `md5sum`, across all
three sample's source directories).

**CNA port behaviour:** Reused ClientServerSample's/NetworkPrediction's
already-converted `Content/Tank.png`, `Turret.png`, `font.font.json`, `font.png`
directly — no reconversion needed, per this repo's own "don't regenerate existing
assets unless there's a confirmed bug" guidance (NetworkPrediction already established
this same reuse for the same source files).

**Root cause:** N/A — same upstream sample family, shared assets.
**Tracked in:** not planned.

## `DrawMessage()` helper dropped (matches ClientServerSample's port, not called out there either)

**XNA behaviour:** `DrawMessage(string message)` is a small helper called right before
the (real, potentially blocking) `NetworkSession.Create()`/`.Find()`/`.Join()` calls,
using `BeginDraw()`/`Clear()`/`spriteBatch.Begin/DrawString/End()`/`EndDraw()` to paint
a "Creating session..."/"Joining session..." status message immediately, before the
call blocks the UI thread.

**CNA port behaviour:** Dropped, matching ClientServerSample's own port (which also
never defines or calls an equivalent method — confirmed by direct inspection of
`samples/ClientServerSample/src/ClientServerGame.hpp`). `NetworkSession::Create()`/
`Find()`/`Join()` all complete synchronously and near-instantly in CNA's current
implementation (no real network handshake to wait on for a `SystemLink`-only,
same-machine session), so there is no actual multi-frame blocking period for this
message to usefully cover.

**Root cause:** N/A — a cosmetic simplification already established by this sample
family's first port, not a CNA gap.
**Tracked in:** not planned.

## Font: "Segoe UI Mono" substituted with DejaVu Sans Mono

**XNA behaviour:** `Font.spritefont` specifies `Segoe UI Mono`, 20pt Regular.
**CNA port behaviour:** Reused ClientServerSample's/NetworkPrediction's already-built
`font.font.json`/`font.png` (generated via `tools/make_font.py` from
`DejaVuSansMono.ttf` at 20px) directly.
**Root cause:** Licensing — not a CNA limitation.
**Tracked in:** not planned.

## Windows Phone / Xbox branches: none exist in the original

**XNA behaviour:** No `#if WINDOWS_PHONE`/Xbox-specific branch exists in this sample
at all — it's already desktop/Xbox-only, using `Keys.A`/`Buttons.A` to create,
`Keys.B`/`Buttons.B` to join, arrow keys + WASD for tank/turret movement.
**CNA port behaviour:** Ported directly, no adaptation needed. `IsPressed()` is
level-triggered (`currentKeyboardState.IsKeyDown || currentGamePadState.IsButtonDown`),
matching this sample's own C# original exactly — note this is a **different** choice
from NetworkPrediction's own rising-edge `IsPressed()`, because the two samples'
actual C# originals genuinely differ on this point (confirmed by direct comparison,
not assumed), same as documented in NetworkPrediction's own missing.md.
**Root cause:** N/A — faithful port.
**Tracked in:** not planned.

## Verification

Confirmed via this repo's established temporary debug-auto-trigger pattern (all
reverted before commit, diff-reviewed to confirm a clean revert):
- Menu screen renders correctly (`"A = create session\nB = join session"`, double-drawn
  black+white, matching the C# original's `DrawMenuScreen()`) — screenshotted with no
  debug code active.
- Session creation (`CreateSession()`) completes with no hang.
- `GamerJoinedEventHandler` fires synchronously — a tank spawns and renders, fully
  textured (`Tank.png`/`Turret.png`), labeled `"Stub Gamer (host)"` (confirming
  `gamer->getIsHostProperty()` correctly reports host status for the local/solo
  gamer, matching the C# original's `if (gamer.IsHost) label += " (host)";`).
- F1 help overlay renders correctly on top of the tank, and correctly disappears
  after its 10-second timer (two screenshots ~6 seconds apart).
- No crash over three separate runs (23s, 30s, and a shorter smoke-test run), each a
  clean from-scratch rebuild.

**Not verified live this session** (same limitation as ClientServerSample/
NetworkPrediction, explicitly acknowledged, not glossed over):
- Real tank movement via keyboard input — `xdotool` reached a real user's window
  (`0x400003`), not this sample's own window, throughout this session (this repo's
  known shared-desktop focus caveat), so no synthetic keypresses were sent.
  `Tank::Update()`'s movement/turning physics are, however, byte-identical code to
  ClientServerSample's own already-verified-live `Tank.hpp` (confirmed via `diff`
  against the C# source), so this is considered proven by that prior verification,
  not re-tested from scratch.
- A genuine multi-machine peer-to-peer test (two real processes exchanging tank state
  over the actual broadcast `SendData()`/`ReceiveData()` path, with two independent
  tanks visible on each machine). Not expected/required for this session, per this
  task's own instructions (same caveat already established for ClientServerSample's
  and NetworkPrediction's own two-process `ENetDiscoveryService` limitation in this
  container). A future session with two real machines/VMs on the same LAN segment
  (or a purpose-built out-of-band port-passing test harness, mirroring `cna`'s own
  `TwoProcessLoopbackTest.cpp`) should confirm the actual N-way broadcast
  synchronization this sample's *own* distinctive mechanic (full-mesh state
  broadcast, no central authority) demonstrates — this is architecturally the one
  aspect of this specific sample that neither ClientServerSample's nor
  NetworkPrediction's own single-gamer verification already covers by analogy, since
  it's the one genuinely new topology question this sample raises.

No new DEFERRED.md item was added — no new CNA gap was found. This sample's own
distinctive mechanic (full-broadcast peer-to-peer state sync with no host authority)
is expressed entirely through already-working `LocalNetworkGamer::SendData(PacketWriter&,
SendDataOptions)` (the broadcast overload, confirmed present and already used
correctly by `UpdateServer()` in ClientServerSample) and `NetworkGamer::ReceiveData()` —
no new API surface was needed.
