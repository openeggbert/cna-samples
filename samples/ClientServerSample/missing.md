# Missing / Differences from XNA 4.0 original

**Status: ported 2026-07-06.** Builds with 0 warnings across all 93 sample targets.
Live-verified via screenshot (`SDL_VIDEODRIVER=x11`): the menu screen renders
correctly ("A = create session" / "B = join session" with drop-shadow text), and
after triggering `CreateSession()`, the session is created, the local gamer's `Tank`
spawns, and the tank/turret sprites render correctly on screen — end to end, no
crash. Porting this (the first of the 4 newly-unblocked networking samples) surfaced
**three real CNA gaps** beyond the "NetworkSession exists and is tested" resolution
already recorded in DEFERRED.md item #17 — see below and DEFERRED.md items #19–21
for the full writeups. This sample works around all three; see the Verification
section at the end for exactly what was and wasn't confirmed live.

Source: `/rv/tmp/XNAGameStudio/Samples/ClientServerSample_4_0/ClientServer/
{ClientServerGame.cs, Tank.cs}`.

## CNA gap: `GamerServicesComponent` + `GamerServicesDispatcher` hangs `NetworkSession::Create`/`Find`/`Join` forever
**XNA behaviour:** The constructor adds a `GamerServicesComponent`
(`Components.Add(new GamerServicesComponent(this));`) — standard boilerplate every
GamerServices-using XNA sample includes.

**CNA port behaviour:** Confirmed live: constructing and adding a
`GamerServicesComponent` before calling `NetworkSession::Create()` makes that call
**hang forever** in a tight busy-loop (99% CPU, unresponsive to SIGTERM). Root cause
(confirmed by reading `cna/src/.../GamerServicesDispatcher.cpp` directly):
`GamerServicesDispatcher::Update()` is a completely empty function body, so
`NetworkSession::Create()`'s synchronous polling loop
(`while (!result->getIsCompletedProperty()) { if (!GamerServicesDispatcher::
UpdateAsync()) activeAction_->setIsCompletedProperty(true); }`) never has anything
set `IsCompleted` once `UpdateAsync()` starts unconditionally returning `true`
(which it does the moment any `GamerServicesComponent` exists). Without a
`GamerServicesComponent`, `UpdateAsync()` returns `false` immediately, forcing
completion on the very first loop iteration — which is why CNA's own
`NetworkSessionTests.cpp` (which never constructs one) doesn't hit this.

**Port-side workaround:** this port does **not** construct/add a
`GamerServicesComponent` at all — a deliberate, documented deviation from the C#
original. Confirmed this loses no functionality (the component has no other
observable effect in this CNA implementation).

**Root cause:** `GamerServicesDispatcher::Update()` needs real completion-polling
logic; currently a no-op stub.

**Tracked in:** DEFERRED.md item #19.

## CNA gap: `NetworkGamer.IsHost` and `.Id` are hardcoded stub constants
**XNA behaviour:** `gamer.IsHost`/`networkSession.IsHost` distinguish the session
host from clients; `gamer.Id` uniquely identifies each gamer within a session, used
by `NetworkSession.FindGamerById()` to route incoming packet data (`byte gamerId =
packetReader.ReadByte(); ... NetworkGamer remoteGamer = networkSession.
FindGamerById(gamerId);`) back to the right `Tank`.

**CNA port behaviour:** confirmed by reading `cna/src/.../NetworkGamer.cpp`:
`getIsHostProperty()` always returns `true` (every gamer, host or client), and
`getIdProperty()` always returns `0` (every gamer). `NetworkSession::
getIsHostProperty()` is defined in terms of the (always-true) per-gamer property, so
`networkSession.IsHost` is also always `true` everywhere. `FindGamerById()` does a
linear `getIdProperty() == id` scan, which — since every gamer's id is 0 — always
returns the *first* gamer in `AllGamers`, regardless of which id was actually
requested.

**Port-side workaround (partial):** for "am I the host," this port tracks a local
`bool isHost_` set explicitly at the call site (`true` after `Create()`, `false`
after `Join()`) instead of trusting `NetworkSession.IsHost`/`gamer.IsHost` — this
fully replaces the broken property for everything this sample needs (deciding
`UpdateServer()` vs. read-only client behavior, and the "(server)" label — though
the label is now only shown next to *this machine's own* gamer, since a genuinely
remote gamer's host status still can't be determined). **No workaround was applied
for `Id`/`FindGamerById`** — the port keeps the original's byte-id wire protocol
as-is. This means **multi-gamer sessions will not correctly route tank state past
the first gamer** in `AllGamers` until item #20 is fixed in `cna`. A solo session
(you create/join and are the only gamer) is unaffected — confirmed live.

**Root cause:** `NetworkGamer` has no real per-instance host/identity state; both
properties are hardcoded stub constants (doc comments say "matching FNA's stub" —
plausible for real FNA/XNA without a live backing service, but blocks correct
multi-gamer behavior here).

**Tracked in:** DEFERRED.md item #20 (IsHost worked around at the sample level; Id/
FindGamerById is not, and remains a real limitation for >1 gamer).

## CNA gap: initial `GamerJoined` event is queued for next frame, not raised synchronously
**XNA behaviour:** `NetworkSession.Create()`/`.Join()` synchronously establish the
initial local gamer(s) and raise `GamerJoined` for each as part of that same call —
by the time `Create()` returns, `e.Gamer.Tag = new Tank(...)` (set inside the
`GamerJoined` handler) has already run.

**CNA port behaviour:** confirmed live — CNA's `NetworkSession` constructor *queues*
a `GamerJoin` event per initial gamer instead of raising it immediately; the queue is
only drained by `NetworkSession::Update()`, i.e. not until the game loop's *next*
`networkSession.Update()` call. Since `UpdateNetworkSession()`'s first action each
frame is to read each local gamer's `Tag` (expecting a `Tank*` already stored there),
the very first frame after `Create()`/`Join()` finds an empty `Tag` and throws
`std::bad_any_cast`, terminating the process. Reproduced and root-caused live via a
temporary debug build before applying the fix below.

**Port-side workaround:** `CreateSession()`/`JoinSession()` each call
`networkSession_->Update();` once, immediately after `HookSessionEvents()` —
draining the queued initial `GamerJoin` event(s) synchronously before returning
control to the normal per-frame loop, matching what real XNA does implicitly.
Confirmed live: with this one extra call, session creation → tank spawn → render all
work correctly with no crash.

**Root cause:** `NetworkSession`'s constructor queues rather than raises the initial
join event(s).

**Tracked in:** DEFERRED.md item #21.

## Windows Phone branch dropped, desktop keyboard/gamepad branch ported as-is
**XNA behaviour:** No `#if WINDOWS_PHONE` branch exists in this sample at all —
it's already desktop/Xbox-only, using `Keys.A`/`Buttons.A` to create and
`Keys.B`/`Buttons.B` to join.
**CNA port behaviour:** Ported directly, no adaptation needed. `IsPressed()`/
`ReadTankInputs()` (arrow keys + WASD for tank/turret aim, normalized) ported
line-for-line.
**Root cause:** N/A — faithful port.
**Tracked in:** not planned.

## `Gamer.SignedInGamers` synthesized locally (CNA's `Guide.ShowSignIn` is a no-op)
**XNA behaviour:** `UpdateMenuScreen()` calls `Guide.ShowSignIn(maxLocalGamers,
false)` when `Gamer.SignedInGamers.Count == 0`, prompting the user to sign into a
local (offline is fine) profile; once signed in, A/B become active.
**CNA port behaviour:** `Guide::ShowSignIn` is a permanent no-op in CNA (confirmed:
`void Guide::ShowSignIn(int, bool) {}`), and nothing else populates
`Gamer::SignedInGamers` — meaning `NetworkSession::Create()`'s constructor would find
zero signed-in gamers and index into an empty `localGamers_[0]` (undefined
behavior/crash). Worked around by synthesizing one `SignedInGamer` directly via
CNA's own `SignedInGamer::CreateInternal("Player", ...)` /
`Gamer::setSignedInGamersProperty(...)` — a CNA-provided (`NOXNA`-tagged) escape
hatch specifically for this situation — in the constructor, before any `Update()`
could try to create/join a session. This reproduces the *state* a real sign-in flow
would have produced; the port also drops the now-unreachable
`Gamer.SignedInGamers.Count == 0` / `Guide.ShowSignIn` branch entirely, since it can
never be true after this.
**Root cause:** `Guide.ShowSignIn` has no interactive UI in CNA to actually produce a
signed-in profile.
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
**Confirmed live (screenshot, this session):** built cleanly (0 warnings, all 93
sample targets unaffected). Menu screen renders correctly. Using a temporary
debug-only auto-trigger (removed before commit — this repo's established pattern for
verifying input-gated flows when `xdotool` can't reliably reach sample windows, per
the `feedback_xdotool_shared_desktop` project memory) to call `CreateSession()`
directly: confirmed the full path — `NetworkSession::Create(SystemLink, ...)` →
`HookSessionEvents()` → flushed `GamerJoined` → `Tank` constructed with real
`Tank.png`/`Turret.png` textures → tank/turret sprites render on screen — with no
crash, over a monitored 8+ second run.

**Not confirmed interactively:** real keyboard-driven 'A'/'B' input (blocked by the
same `xdotool` unreliability documented across this repo's other samples — a
follow-up screenshot after a synthetic keypress showed no state change, and this
desktop's window/X-server state was independently observed to be flaky mid-session,
consistent with prior sessions' findings, not a code bug). **Not confirmed at all
this session:** the actual `JoinSession()`/multi-machine path (would need two
separate running instances discovering each other over real LAN broadcast — not
attempted, given the above). A future session with reliable local input, or two
machines/VMs on the same network segment, should confirm: (a) `JoinSession()` finds
and joins a session created by a separate instance, and (b) tank movement/state
actually synchronizes between host and client (this second point is exactly where
item #20's `Id`/`FindGamerById` gap would first become observable, once a second
gamer is actually in the session).
