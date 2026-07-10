# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-10.** Builds with 0 warnings. This is the first attempt at
"section 8 task 7" (NEXT.md) — a direct test of whether ClientServerSample's (#091)
three networking workarounds (DEFERRED.md items #19/#20/#21, all resolved upstream in
`cna`/`sharp-runtime` since 2026-07-06) are genuinely gone for a *different* sample's
own `Update()`-loop shape, not just ClientServerSample's own. **Confirmed: this port
needed zero of those three workarounds** — a real `GamerServicesComponent` is
constructed normally in the constructor (matching the C# original exactly),
`networkSession_->getIsHostProperty()`/`gamer->getIsHostProperty()` are used directly
with no locally-tracked bool, and `HookSessionEvents()` needs no extra manual
`Update()` call afterward (`GamerJoined` replays immediately on subscription). See the
dedicated section below and Verification for the live proof.

This sample's own real engineering content turned out to be a **new, fourth CNA
networking gap**, distinct from items #19–21: `NetworkSession::SessionProperties` has
no mutable accessor and is never replicated over the wire (new DEFERRED.md item #27) —
see its own section below for the full account and the workaround applied.

Source: `/rv/tmp/XNAGameStudio/Samples/NetworkPredictionSample_4_0/NetworkPrediction/
{NetworkPredictionGame.cs, Tank.cs, RollingAverage.cs}`.

## Confirmed: zero of ClientServerSample's three original networking workarounds were needed

**XNA behaviour:** `NetworkPredictionGame()`'s constructor adds a
`Components.Add(new GamerServicesComponent(this));`, exactly like ClientServerSample's
and every other GamerServices-using XNA sample's boilerplate.

**CNA port behaviour:** Ported line-for-line —
`gamerServices_ = std::make_unique<GamerServicesComponent>(*this);
getComponentsProperty().Add(gamerServices_.get());` in the constructor, with no
workaround of any kind:

- **Item #19 (`GamerServicesDispatcher::Update()` no-op hang):** fixed upstream before
  this sample was ever attempted (`cna` `feature/net` commit `08171ac`, merged to
  `develop` well before this session). `NetworkSession::Create()` completes
  synchronously with no hang, confirmed live (see Verification).
- **Item #20 (`NetworkGamer.IsHost`/`.Id` hardcoded stubs):** also fixed upstream.
  `UpdateNetworkSession()`/`UpdateOptions()`/`SendOptionsPacket()` all call
  `networkSession_->getIsHostProperty()` directly, with no local `bool isHost_`
  tracking member anywhere in this port — confirmed live the session correctly
  identifies itself as host (`DrawOptions()`'s "(A to change)" etc. prompts render,
  which only appear when `getIsHostProperty()` is true).
- **Item #21 (initial `GamerJoined` queued, not synchronous):** also fixed upstream
  (`sharp-runtime` `EventHandler<T>::SetReplayHook()`, commit `69661c2`). This port's
  `CreateSession()`/`JoinSession()` call `HookSessionEvents()` and nothing else — no
  extra `networkSession_->Update();` call is needed afterward, and none was added.
  Confirmed live: the very first frame after `CreateSession()` already has a fully
  populated `Tank` on the local gamer's `Tag` (proven by `UpdateLocalGamer()`'s
  `std::any_cast<Tank*>` not throwing on that same frame — the exact failure mode this
  bug would have caused if still present).

**Root cause / why this differs from ClientServerSample's own experience:** those three
bugs were real, `cna`-side framework gaps, not something specific to
ClientServerSample's own code shape — once fixed in `cna`/`sharp-runtime`, *any*
sample using the same `NetworkSession`/`GamerServicesComponent` APIs benefits
automatically. This port is the first direct confirmation of that generality on a
second, independently-written sample.

**Tracked in:** DEFERRED.md items #19/#20/#21 (already ✅ resolved; this entry is
confirmation, not a new finding).

## New CNA gap: `NetworkSession.SessionProperties` has no mutable accessor and is never replicated over the wire

**XNA behaviour:** `UpdateOptions()` (`NetworkPredictionGame.cs:386-451`) is the heart
of this sample's whole point — the host computes 4 settings from local input (which
`NetworkQuality` to simulate, the packet send rate, and whether prediction/smoothing
are enabled) and writes them with
`networkSession.SessionProperties[0] = (int)networkQuality; ... [1] = framesBetweenPackets; ...`.
Every other machine in the session reads the *same* indices back
(`networkSession.SessionProperties[0]`, etc.) and sees the host's latest values with
**no explicit packet-send call anywhere in the sample's own code** — real XNA's
networking layer silently, automatically replicates this list to every peer.

**CNA port behaviour:** Confirmed by direct source read (both
`include/Microsoft/Xna/Framework/Net/NetworkSession.hpp` and the matching `.cpp`) that
neither half of this exists:

1. `NetworkSession::getSessionPropertiesProperty()` returns `const
   NetworkSessionProperties&` — no non-const overload, no setter, no other way to reach
   a mutable reference to the session's own properties list from outside the class.
   (`NetworkSessionProperties::operator[]` *does* have a working mutable overload — it's
   simply unreachable through `NetworkSession`'s own const-only getter.)
2. Even granting a hypothetical mutable accessor, `sessionProperties_` is set exactly
   once, in the constructor's member-initializer list, and is never read or written
   anywhere else in `NetworkSession.cpp` — there is no replication logic of any kind,
   confirmed by grepping the whole file for every reference to it.

**Root cause:** A missing, two-part `cna` feature (no mutation path, no replication
mechanism), not a bug in already-existing code — `NetworkSession.SessionProperties`
was evidently never exercised by any sample or test that writes to it after
construction before this port.

**Workaround applied in this port (NOXNA, no C# equivalent):** the host still computes
the same 4 settings from local input in `UpdateOptions()`, unchanged in shape from the
original. Instead of writing them into `SessionProperties`, it broadcasts them
explicitly in a small "options packet," using the same
`LocalNetworkGamer::SendData`/`ReceiveData` channel already used for tank-state
packets, distinguished by a new leading `PacketKind` byte (`TankState = 0` /
`Options = 1`, see `NetworkPredictionGame.hpp`'s `PacketKind` enum and
`SendOptionsPacket()`/`ReadIncomingPackets()`):

- The host sends an options packet at the same throttled cadence as tank packets
  (`sendPacketThisFrame`), via `(LocalNetworkGamer*)networkSession_->getHostProperty()`.
- `ReadIncomingPackets()` checks the leading byte on every received packet; a
  `TankState` packet is handled exactly as in the C# original (dispatched into
  `Tank::ReadNetworkPacket()`), while an `Options` packet updates this machine's own
  `networkQuality_`/`framesBetweenPackets_`/`enablePrediction_`/`enableSmoothing_`
  fields directly, standing in for the client-side branch of the original
  `UpdateOptions()` that used to read `networkSession.SessionProperties[i]`.
- `UpdateOptions()`'s tail (`switch (networkQuality_) { ... setSimulatedLatencyProperty
  (...); setSimulatedPacketLossProperty(...); }`) runs unconditionally on both host and
  client, using whichever local copy of `networkQuality_` is currently authoritative on
  that machine (self-controlled key input on the host; the most recently received
  options packet on a client) — `NetworkSession::SimulatedLatency`/`SimulatedPacketLoss`
  themselves are real, already-working CNA properties (confirmed via
  `getSimulatedLatencyProperty()`/`setSimulatedLatencyProperty()`/
  `getSimulatedPacketLossProperty()`/`setSimulatedPacketLossProperty()` in
  `NetworkSession.hpp`) — only the *settings-replication* mechanism needed a
  workaround, not the simulated-latency/packet-loss feature itself.

This reproduces the original's visible behavior (host changes a setting, every machine
eventually sees and applies the same setting) using an explicit packet instead of
implicit property replication — a real, but narrow and clearly-isolated, deviation
confined to one `enum class PacketKind` and two small private methods, with `Tank.hpp`
itself completely unaffected (its `WriteNetworkPacket`/`ReadNetworkPacket` methods
match `Tank.cs` line-for-line, with no `PacketKind` byte inside `Tank` at all — the
type byte is written/read one level up, in the outer game class, before/after
delegating into `Tank`).

**Verification:** confirmed live for the one-machine case this session could test (see
Verification section below) — a solo/single-gamer session created via
`NetworkSession::Create()` renders the correct `DrawOptions()` text
(`"Packets per second = 10 (B to change)"`, `"Prediction = on (X to toggle)"`, etc.)
driven by these locally-tracked fields, matching what the original's
`SessionProperties`-based mechanism would have shown for a lone host with no clients
yet. **Not verified this session:** the actual host → client replication path with a
second live instance (the same "no genuine 2-machine LAN test" limitation every
networking sample in this repo carries — see DEFERRED.md items #19–21's own
verification notes).

**Tracked in:** DEFERRED.md item #27 (new).

## `Guide.ShowSignIn` is a no-op; the check it guards is dead code in practice (same as ClientServerSample)

**XNA behaviour:** `UpdateMenuScreen()` calls `Guide.ShowSignIn(maxLocalGamers, false)`
when `Gamer.SignedInGamers.Count == 0`, prompting the user to sign into a local
profile before A/B become active.

**CNA port behaviour:** Ported the same `if (Gamer::getSignedInGamersProperty()->
getCountProperty() == 0) Guide::ShowSignIn(...); else if (IsPressed(A)) ... else if
(IsPressed(B)) ...` structure directly (unlike ClientServerSample's own port, which
omitted this branch entirely and documented why) — kept here for closer fidelity to
the C# original's code shape, since it costs nothing: `GamerServicesComponent`'s real
`Initialize()` (see the confirmed-resolved item #19 section above) populates
`Gamer::SignedInGamers` with 4 stub gamers before this game's first `Update()` call, so
`Count` is never 0 and this branch is never actually taken — functionally identical to
ClientServerSample's own omission, just written out explicitly.

**Root cause:** `Guide.ShowSignIn` has no interactive UI in CNA to actually produce a
signed-in profile (same as ClientServerSample's own finding) — not this session's
scope to fix.

**Tracked in:** not planned — see `samples/ClientServerSample/missing.md`'s own
identical finding; same root cause, not re-diagnosed.

## Texture conversion: Tank.tga / Turret.tga → PNG (reused byte-identical ClientServerSample assets)

**XNA behaviour:** `Content/Tank.tga`, `Content/Turret.tga`.

**CNA port behaviour:** Confirmed via `md5sum` that this sample's `Tank.tga`/
`Turret.tga` are **byte-identical** to ClientServerSample's own already-converted
source assets (`98b800c...`/`ace0e08...` match exactly) — copied the already-converted
`Content/Tank.png`/`Turret.png` directly from `samples/ClientServerSample/Content/`
instead of reconverting, per this repo's own "don't regenerate existing assets unless
there's a confirmed bug" guidance (NEXT.md section 9).

**Root cause:** CNA's asset pipeline doesn't support `.xnb`/TGA source assets (see
CLAUDE.md's Assets section) — same as every other sample.

**Tracked in:** not planned.

## Font: "Segoe UI Mono" substituted with DejaVu Sans Mono (reused byte-identical ClientServerSample asset)

**XNA behaviour:** `Font.spritefont` specifies `Segoe UI Mono`, 20pt Regular.

**CNA port behaviour:** Confirmed via `diff` that this sample's `Font.spritefont` is
byte-identical to ClientServerSample's own — copied the already-generated
`font.font.json`/`font.png` atlas directly (`tools/make_font.py`'s own DejaVu Sans
Mono substitution, unchanged) instead of regenerating. `Content.Load<SpriteFont>
("font")` uses the same lowercase content name ClientServerSample's own port
established, even though the C# original calls `Content.Load<SpriteFont>("Font")`
(capital F) — matches this repo's existing precedent for this identical asset.

**Root cause:** Licensing — not a CNA limitation.

**Tracked in:** not planned.

## Windows Phone branch dropped, desktop keyboard/gamepad branch ported as-is

**XNA behaviour:** No `#if WINDOWS_PHONE` branch exists in this sample — already
desktop/Xbox-only. Keyboard controls: `A`/`B` to create/join a session (menu screen)
or change network-quality/send-rate settings (in-session, host only); `X`/`Y` to
toggle prediction/smoothing; arrow keys to drive the tank; `K`/`O`/`L`/`;`
(OemSemicolon) to aim the turret.

**CNA port behaviour:** Ported directly, no adaptation needed — `IsPressed()`/
`ReadTankInputs()`/`UpdateOptions()`'s input handling all match `NetworkPredictionGame.cs`
line-for-line, including the **rising-edge** (not level-triggered) semantics of
`IsPressed()` (`currentState.IsKeyDown && previousState.IsKeyUp`) — a deliberate,
faithful difference from ClientServerSample's own port, whose `IsPressed()` is
level-triggered (matching *its own* C# original, which really is level-triggered
there). Confirmed by direct comparison of both C# originals: ClientServerSample's
`IsPressed()` has no `previousKeyboardState`/`previousGamePadState` fields at all,
while `NetworkPredictionGame.cs` explicitly tracks and compares against them — two
different samples with two genuinely different original behaviors, both ported
faithfully rather than unified.

**Root cause:** N/A — faithful port.

**Tracked in:** not planned.

## `cna-samples` build wiring: no new gap (inherited from ClientServerSample)

**What was needed:** `CNA_ENABLE_NET`/`CNA_Net` linking, exactly as ClientServerSample's
port already fixed in this repo's root `CMakeLists.txt`/`cmake/SampleHelpers.cmake`.
No further change was needed — this sample's own `CMakeLists.txt` is the standard
`cna_add_sample(NetworkPrediction SOURCES src/Program.cpp CONTENT_DIR ...)` shape, and
linking `CNA_Net` happens automatically for every sample now.

**Root cause:** N/A — already fixed.

**Tracked in:** N/A (already fixed; confirmed still working here).

## Verification

**2026-07-10:** Built via
`cmake --build cmake-build-debug --target NetworkPrediction_cna_samples -j$(nproc)` —
0 errors, 0 warnings, confirmed via two from-scratch object-file rebuilds (object file
deleted and rebuilt both times, output grepped for "warning"/"error", none found both
times).

Ran under `SDL_VIDEODRIVER=x11`. `xdotool getactivewindow` showed a different real
user window (`0x400003`) held focus throughout this session (this repo's known
shared-desktop `xdotool` caveat — NEXT.md section 5) — no synthetic keypresses were
sent to this sample's own window. A clean run (no debug code) confirmed the menu
screen renders correctly ("A = create session" / "B = join session", white-on-black
drop-shadow text) and the process survives several seconds with no crash.

Used this repo's own established temporary debug-auto-trigger pattern (a `bool
debugAutoCreate_ = true;` field and a 6-line `if` block in `Update()` that call
`CreateSession()` and force `helpTimer_ = 10.0f` on the very first frame — both
reverted before commit, confirmed via a subsequent clean from-scratch rebuild with 0
warnings) to verify, live, in a single combined screenshot:

- `NetworkSession::Create()` completes with no hang (item #19's fix holds for this
  sample too).
- `GamerJoinedEventHandler` fires **synchronously** off the very first `Update()` call
  with no manual `networkSession_->Update()` needed anywhere (item #21's fix holds) —
  a `Tank` renders fully textured (tank body + turret sprites, both correctly loaded
  from the copied `Tank.png`/`Turret.png`) with the gamertag label `"Stub Gamer"`
  above it, at the position `Tank`'s constructor computes for gamer index 0.
- `networkSession_->getIsHostProperty()` correctly reports `true` for this
  solo-created session (item #20's fix holds) — confirmed indirectly via
  `DrawOptions()`'s text, which only appends the "(X to change)"-style prompts when
  `IsHost` is true, and those prompts are visible in the screenshot
  (`"...Prediction = on (X to toggle)"`, `"...Smoothing = on (Y to toggle)"`, and a
  correctly-computed `"...econd = 10 (B to..."` reflecting the default
  `framesBetweenPackets_ = 6` → `60/6 = 10` packets/sec).
- The F1 help overlay renders correctly on top of everything else — the semi-
  transparent white panel with the correct 9-row control table extracted from
  `NetworkPrediction.htm` by `tools/gen_help_png.py` (no one-off column-selection
  variant needed; this `.htm`'s table has the standard 3 columns).

Process ran 9+ seconds total across two separate launches (once with the debug
trigger, once clean afterward) with no crash. Two-machine `JoinSession()`/replication
of `SendOptionsPacket()`'s actual host → client wire path were **not** tested live
this session — same "no genuine 2-process LAN test" limitation ClientServerSample's
own `missing.md` already documents in detail (its own attempted 2-instance test hit a
pre-existing `ENetDiscoveryService` two-process discovery limitation on this
container, not a regression, and not re-attempted here since it isn't specific to this
sample).
