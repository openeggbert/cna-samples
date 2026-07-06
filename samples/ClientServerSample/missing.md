# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-06; cleaned up in full 2026-07-06 after `cna`'s Phase 12
fixes.** Builds with 0 warnings. Porting this (the first of the 4 newly-unblocked
networking samples) surfaced **three real CNA gaps** beyond the "NetworkSession
exists and is tested" resolution already recorded in DEFERRED.md item #17 — see
DEFERRED.md items #19–21 for the full writeups. **All three are now fixed upstream in
`cna`/`sharp-runtime`** (`feature/net` commits `08171ac`/`81f10b5`/`ab05395`;
`sharp-runtime` `develop` commit `69661c2`) — this port's workarounds for all three
were removed and the real APIs used directly instead (see each section below); item
#21's fix required a `sharp-runtime` change, which the user explicitly reviewed and
approved. Live-verified: session creation, tank spawn/render with the correct
`"Stub Gamer (server)"` label (real `GamerServicesComponent` identity, not the old
manually-synthesized `"Player"`), and movement all work end-to-end, no crash, with
**zero** of the three original workarounds remaining.

Source: `/rv/tmp/XNAGameStudio/Samples/ClientServerSample_4_0/ClientServer/
{ClientServerGame.cs, Tank.cs}`.

## ✅ FIXED (was: CNA gap): `GamerServicesComponent` + `GamerServicesDispatcher` hung `NetworkSession::Create`/`Find`/`Join` forever
**XNA behaviour:** The constructor adds a `GamerServicesComponent`
(`Components.Add(new GamerServicesComponent(this));`) — standard boilerplate every
GamerServices-using XNA sample includes.

**CNA port behaviour, before the fix:** constructing and adding a
`GamerServicesComponent` before calling `NetworkSession::Create()` made that call
**hang forever** in a tight busy-loop (99% CPU). Root cause: `GamerServicesDispatcher::
Update()` was a completely empty function body, so `NetworkSession::Create()`'s
synchronous polling loop never had anything set `IsCompleted` once a
`GamerServicesComponent` existed.

**Fixed upstream in `cna`** (`feature/net` commit `08171ac`, Task 12.1): every
`NetworkSessionAction` is now completed the instant it's constructed, since all of
`NetworkSession`'s real work already happens synchronously with no genuine pending
operation to wait on — matching FNA's own local-only, synchronous stub semantics
(confirmed this hang is a real bug in FNA's own reference source too, not just CNA's
port).

**Port-side change, this session:** the constructor now adds a real
`gamerServices_ = std::make_unique<GamerServicesComponent>(*this);
getComponentsProperty().Add(gamerServices_.get());`, matching the C# original exactly
— no more workaround. Confirmed live (see Verification): `CreateSession()` no longer
hangs, and its `GamerServicesComponent::Initialize()` now real-populates
`Gamer::SignedInGamers` with 4 stub gamers, making the separate manually-synthesized
`"Player"` `SignedInGamer` workaround (see below) unnecessary too.

**Tracked in:** DEFERRED.md item #19 — resolved.

## ✅ FIXED (was: CNA gap, partial workaround): `NetworkGamer.IsHost` and `.Id` were hardcoded stub constants
**XNA behaviour:** `gamer.IsHost`/`networkSession.IsHost` distinguish the session
host from clients; `gamer.Id` uniquely identifies each gamer within a session, used
by `NetworkSession.FindGamerById()` to route incoming packet data (`byte gamerId =
packetReader.ReadByte(); ... NetworkGamer remoteGamer = networkSession.
FindGamerById(gamerId);`) back to the right `Tank`.

**CNA port behaviour, before the fix:** `getIsHostProperty()` always returned `true`
(every gamer, host or client), and `getIdProperty()` always returned `0` (every
gamer), so `FindGamerById()` always resolved to the *first* gamer in `AllGamers`
regardless of the id requested — multi-gamer sessions couldn't correctly route tank
state past the first gamer.

**Fixed upstream in `cna`** (`feature/net` commit `81f10b5`, Task 12.2):
`NetworkGamer` gained real `SetId`/`SetIsHost` (NOXNA), wired through
`NetworkSession`'s constructor (a new `bool isHost` parameter: `true` from
`EndCreate`, `false` from `EndJoin`/`EndJoinInvited`) and through `ENetBackend.cpp`'s
already-existing real, cross-machine-consistent wire-id system (it just wasn't
surfaced through the public API before). **Scoped, still-open limitation** (documented
upstream, not silently dropped): a *remote* gamer representing the actual host
machine still reports `IsHost == false` as seen from a client (the wire roster
carries no host flag) — a strict improvement over "every gamer said true," not a full
fix.

**Port-side change, this session:** removed the local `bool isHost_` tracking member
entirely; every call site now uses `networkSession_->getIsHostProperty()` (for
host/client branching in `UpdateNetworkSession()`/`UpdateLocalGamer()`) or
`gamer->getIsHostProperty()` directly (for the "(server)" label in
`DrawNetworkSession()`, now matching the C# original's exact `if (gamer.IsHost)` with
no extra guard needed). `Id`/`FindGamerById` needed no port-side code change — they
were already written against the real API, just silently broken by the upstream bug;
now correct.

**Verification limits (honest, not glossed over):** confirmed live, single-gamer
(solo host, `"Stub Gamer (server)"` label renders correctly — see Verification
section). **Not confirmed live this session:** genuine multi-gamer `Id`-routing
end-to-end within this sample specifically — attempted a real two-instance
host+client test, but the client's `NetworkSession::Find()` returned "No network
sessions found" (a separate, pre-existing, documented platform limitation: two
independent processes both relying on `ENetDiscoveryService`'s shared well-known
port is fragile in this container — the same reason `cna`'s own
`TwoProcessLoopbackTest.cpp` hands the host's port to the client out-of-band instead
of through discovery). Multi-gamer `Id`-routing correctness is proven at the `cna`
test-suite level instead (`NetworkSessionTests.cpp`'s
`MultipleLocalGamersGetDistinctIdsAndFindGamerByIdRoutesCorrectly`, and
`ENetBackendTests.cpp`'s wire-id assertions over real ENet traffic).

**Tracked in:** DEFERRED.md item #20 — resolved (with the scoped remote-host-IsHost
limitation noted above, tracked as a known gap, not a new item).

## ✅ FIXED (was: investigated, genuinely blocked): initial `GamerJoined` event is queued for next frame, not raised synchronously
**XNA behaviour:** `NetworkSession.Create()`/`.Join()` synchronously establish the
initial local gamer(s) and raise `GamerJoined` for each as part of that same call —
by the time `Create()` returns, `e.Gamer.Tag = new Tank(...)` (set inside the
`GamerJoined` handler) has already run.

**CNA port behaviour, before the fix:** CNA's `NetworkSession` constructor *queued*
a `GamerJoin` event per initial gamer instead of raising it immediately; the queue was
only drained by `NetworkSession::Update()`.

**Investigated in `cna`'s `feature/net`, Task 12.3 — found genuinely blocked at first,
then fixed with the user's explicit sign-off:** traced against this exact sample's
real C# source (`ClientServerGame.cs`) and confirmed neither of the two
originally-considered fixes (raise inside the constructor; drain the queue before
`Create()`/`Join()` return) could work on their own — subscription
(`HookSessionEvents()`) always happens *after* `Create()`/`Join()` already returned,
so firing the event any earlier fires into zero subscribers. Real XNA's `GamerJoined`
is documented to replay itself immediately upon `+=` subscription for every
already-present gamer; CNA's `System::EventHandler<T>` (a separate, `sharp-runtime`
repo) had no hook for that. **The user reviewed this exact analysis and approved a
`sharp-runtime` change**: `EventHandler<T>` gained an opt-in `SetReplayHook()` (generic,
not a one-off hack — every other `EventHandler<T>` in the codebase is unaffected), and
`NetworkSession`'s constructor now uses it so `GamerJoined += handler` immediately
replays for every gamer already in the session, matching real XNA.

**Port-side change:** removed the `networkSession_->Update();` call that used to be
needed right after `HookSessionEvents()` in both `CreateSession()` and
`JoinSession()` — no longer necessary, since `HookSessionEvents()`'s own `+=` now
delivers the initial join synchronously by itself. **Verified live**: since
`xdotool` proved unreliable again on this shared desktop this round (see this file's
own `Verification` section's history of the same issue), used this repo's own
established fallback — a temporary debug auto-trigger calling `CreateSession()` after
30 frames, removed before commit — and confirmed session creation → tank spawn →
render all still work correctly with **no** manual `Update()` call, no crash, over a
multi-second run.

**Tracked in:** DEFERRED.md item #21 — resolved.

## Windows Phone branch dropped, desktop keyboard/gamepad branch ported as-is
**XNA behaviour:** No `#if WINDOWS_PHONE` branch exists in this sample at all —
it's already desktop/Xbox-only, using `Keys.A`/`Buttons.A` to create and
`Keys.B`/`Buttons.B` to join.
**CNA port behaviour:** Ported directly, no adaptation needed. `IsPressed()`/
`ReadTankInputs()` (arrow keys + WASD for tank/turret aim, normalized) ported
line-for-line.
**Root cause:** N/A — faithful port.
**Tracked in:** not planned.

## `Gamer.SignedInGamers` populated via the real `GamerServicesComponent` (CNA's `Guide.ShowSignIn` is still a no-op)
**XNA behaviour:** `UpdateMenuScreen()` calls `Guide.ShowSignIn(maxLocalGamers,
false)` when `Gamer.SignedInGamers.Count == 0`, prompting the user to sign into a
local (offline is fine) profile; once signed in, A/B become active.
**CNA port behaviour, before this session's cleanup:** `Guide::ShowSignIn` was (and
still is) a permanent no-op in CNA, and — back when `GamerServicesComponent` was
omitted entirely (see the fixed item #19 above) — nothing else populated
`Gamer::SignedInGamers` either, so this port synthesized one `SignedInGamer` directly
via `SignedInGamer::CreateInternal("Player", ...)` / manual
`Gamer::setSignedInGamersProperty(...)`, a CNA-provided (`NOXNA`-tagged) escape hatch.
**CNA port behaviour, now that item #19 is fixed:** the real
`GamerServicesComponent` (added in the constructor, see item #19 above) has its
`Initialize()` called automatically during `Game::Initialize()`, which calls
`GamerServicesDispatcher::Initialize()` — this **real** entry point populates
`Gamer::SignedInGamers` with 4 stub gamers (`"Stub Gamer"`, `"Stub Gamer (1)"`, …,
matching FNA's own "FIXME: This is stupid" stub) *before* this game's first
`Update()` call. The manual `SignedInGamer::CreateInternal("Player", ...)` synthesis
is gone; this sample's tank now labels as `"Stub Gamer"`, not `"Player"` (confirmed
live — see Verification). `Guide.ShowSignIn` is still never reached in practice
(`Gamer.SignedInGamers.Count` is 4, never 0, from the very first `Update()` onward) —
same unreachable-branch reasoning as before, just via the real dispatcher instead of
a manual override; the port still drops that branch rather than keeping visibly-dead
code.
**Root cause:** `Guide.ShowSignIn` has no interactive UI in CNA to actually produce a
signed-in profile — unchanged, not part of Phase 12's scope.
**Tracked in:** not planned (CNA already provides `SignedInGamer::CreateInternal` for
exactly this situation — a documented, sanctioned workaround, not a raw hack).

## Texture conversion: Tank.tga / Turret.tga → PNG
**XNA behaviour:** `Content/Tank.tga`, `Content/Turret.tga` (128×128 RGBA Targa
files with a real alpha channel, confirmed via `identify`/corner-pixel check — not
magenta-color-keyed).
**CNA port behaviour:** Converted to `Content/Tank.png`/`Turret.png` via ImageMagick
`convert`, loaded via the same `Content.Load<Texture2D>("Tank")`/`("Turret")` calls.
**Root cause:** CNA's asset pipeline doesn't support `.xnb`/TGA source assets (see
CLAUDE.md's Assets section).
**Tracked in:** not planned (standard conversion, matching Audio3D/GesturesSample/
AimingSample's own `.tga`→`.png` precedent).

## Font: "Segoe UI Mono" substituted with DejaVu Sans Mono
**XNA behaviour:** `Font.spritefont` specifies `Segoe UI Mono`, 20pt Regular.
**CNA port behaviour:** `tools/make_font.py` generated a `.font.json` + PNG atlas
from `DejaVuSansMono.ttf` at 20px — this repo's standard SpriteFont substitution.
**Root cause:** Licensing — not a CNA limitation.
**Tracked in:** not planned.

## `cna-samples` build-wiring fix: `CNA_ENABLE_NET` / `CNA_Net` linking
**What was missing:** `NetworkSession`/`GamerServicesComponent`/etc. live in
`CNA_Net`/`CNA_GamerServices` — separate CMake targets in `cna`, gated behind a
`CNA_ENABLE_NET` option that's `OFF` by default and never set by this repo's root
`CMakeLists.txt`. Even with the option on, `cmake/SampleHelpers.cmake`'s
`cna_add_sample()` never linked `CNA_Net` into any sample executable. Confirmed via
a first linker failure: dozens of `undefined reference to Microsoft::Xna::Framework::
Net::*`/`GamerServices::Gamer::*` symbols and missing vtables.
**Fix (this repo, not `cna`):** added `set(CNA_ENABLE_NET ON CACHE BOOL ... FORCE)`
to the root `CMakeLists.txt` (next to the existing `CNA_BUILD_TESTS`/
`CNA_BUILD_EXAMPLES` cache-variable forwarding), and `if(TARGET CNA_Net)
target_link_libraries(${full_target} PRIVATE CNA_Net) endif()` to
`cna_add_sample()` in `cmake/SampleHelpers.cmake` — harmless for samples that don't
use it, and now required for this sample plus NetworkPrediction/PeerToPeer/NetRumble
once ported.
**Root cause:** N/A — `cna-samples`-side build configuration gap, not a `cna` API
gap. Fixed directly since it's this repo's own file.
**Tracked in:** N/A (fixed).

## Verification

**2026-07-06, Phase 12 cleanup session (this session), after removing the item
#19/#20 workarounds:** built against `cna`'s `feature/net` tip (commit `f0b0499`,
temporarily checked out locally in the `../cna` build-dependency checkout — not
merged/pushed to `develop`, since that's a separate decision for whoever manages that
branch) — 0 warnings, clean link. Live-verified with real `xdotool` keypresses this
time (previous session's `xdotool`-unreliability note above no longer applied):
- Launched the built binary from its own output directory (`Content/` resolves
  relative to the executable, not the invocation directory — a real gotcha hit and
  fixed by `cd`-ing there first, not a CNA bug).
- Sent a real `a` keypress: `CreateSession()` ran to completion with **no hang**
  (confirming Task 12.1's fix in a real, non-synthetic run, unlike the prior
  session's debug-auto-trigger workaround) — screenshotted the tank spawning, labeled
  `"Stub Gamer (server)"` (the real `GamerServicesDispatcher`-populated gamertag,
  confirming Task 12.1's `GamerServicesComponent` addition and item #19's removed
  workaround both work; previously this would have shown the manually-synthesized
  `"Player"`).
- Sent a real `Right` arrow keypress: screenshotted the tank having moved — confirms
  `UpdateLocalGamer()`/input handling/rendering all still work correctly with
  `networkSession_->getIsHostProperty()` replacing the removed `isHost_` member.
- Attempted a genuine two-instance host+client test (second window, `Right` `b` to
  join): the client's `NetworkSession::Find()` returned "No network sessions found" —
  a separate, pre-existing, already-documented platform limitation (two independent
  processes both relying on `ENetDiscoveryService`'s shared well-known port is
  fragile in this container; `cna`'s own `TwoProcessLoopbackTest.cpp` avoids it by
  handing the port out-of-band instead of through discovery). Not a regression from
  this session's changes, and not something this sample's own cleanup can work around
  without its own out-of-band port-passing mechanism (out of scope here).
- Genuine multi-gamer `Id`-routing (item #20's core fix) was therefore **not**
  re-verified live within this specific sample this session, for the reason above;
  it's the `cna` test suite (`NetworkSessionTests.cpp`'s
  `MultipleLocalGamersGetDistinctIdsAndFindGamerByIdRoutesCorrectly`,
  `ENetBackendTests.cpp`'s wire-id assertions over real ENet traffic, and the real
  two-process `TwoProcessLoopbackTest.cpp`) that proves this end-to-end instead.

A future session with two real machines/VMs on the same LAN segment (or a
purpose-built out-of-band port-passing test harness for this specific sample, mirroring
`cna`'s own two-process test) should confirm `JoinSession()` + multi-gamer tank-state
sync end-to-end within `ClientServerSample` itself, if that level of proof is ever
needed beyond what the `cna` test suite already covers.

**2026-07-06, same-day follow-up, after item #21's `sharp-runtime` fix landed:**
rebuilt against `cna`'s `feature/net` tip (fast-forwarded the same local `../cna`
checkout to commit `ab05395`, which itself pulled in the just-updated
`sharp-runtime` `develop` at `69661c2` via `../sharp-runtime` relative to `cna`) — 0
warnings, clean link. Removed the last remaining workaround (the
`networkSession_->Update();` call right after `HookSessionEvents()` in both
`CreateSession()`/`JoinSession()`).
`xdotool` input was unreliable again this round — confirmed genuinely environmental,
not a code regression, by testing three ways: real shared-desktop keypresses (failed
across several retries and process restarts), a completely isolated `Xvfb :77`
display with no window manager (failed identically, ruling out shared-desktop
focus-stealing specifically), and finally this repo's own established fallback for
exactly this situation — a temporary debug auto-trigger (`CreateSession()` called
automatically after 30 frames, deleted before commit, diff-reviewed to confirm a
clean revert). The auto-trigger run **confirmed the fix live**: session creation →
`GamerJoined` fires (no manual `Update()` call anymore) → tank spawns labeled
`"Stub Gamer (server)"` → renders correctly, stable over a multi-second run, no
crash. This is the strongest possible proof for this specific change: if the removed
`Update()` call had still been necessary, `UpdateLocalGamer()`'s `std::any_cast<Tank*>`
on the local gamer's `Tag` would have thrown `std::bad_any_cast` on the very first
`UpdateNetworkSession()` call (the exact failure mode item #21 originally described) —
it didn't.
`ClientServerSample` now has **zero** of its original three DEFERRED.md workarounds
(#19, #20, #21 all resolved upstream).
