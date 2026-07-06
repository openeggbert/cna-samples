# Missing / Differences from XNA 4.0 original

This is the largest sample ported in this repo (~32,700 lines of C# across
`RolePlayingGame`/`RolePlayingGameData`/`RolePlayingGameProcessors`). The
deviations below are organized by how significant they are, starting with the
one the user explicitly approved before this port began.

## XML data loader replaces the compiled content pipeline (major, approved deviation)

**XNA behaviour:** All 281 data files (armor/weapons/monsters/maps/quests/
chests/stores/etc.) are hand-authored XML in the stock XNA content-pipeline
"IntermediateSerializer" format (`<Asset Type="RolePlayingGameData.X">`,
element name == C# property name, matched via reflection at content-build
time). The build compiles each into a binary `.xnb`; at runtime,
`RolePlayingGameData`'s `*Reader : ContentTypeReader<T>` classes then read
that binary *positionally* (`input.ReadInt32()`, `input.ReadString()`, ... in
a fixed field order matching the original property declaration order).

**CNA port behaviour:** This project's established convention for the rare
custom-XML sample (DynamicMenu's `MenuPage2.xml`, NinjAcademy's
`Animations.xml`/`Configuration.xml`) is to hand-translate the XML into C++
construction code. That does not scale to 281 files. Per the user's explicit
direction for this sample, `Data/ContentLoader.hpp` instead parses the XML
directly at runtime via a small hand-rolled DOM parser (`Xml/XmlNode.hpp`,
~150 lines, handles exactly the well-formed dialect these files use: nested
elements, a single `Type` attribute, `<Item>`-tagged list children, and
whitespace-separated-number text for `Point`/`Vector2`/int arrays) and
matches each field **by element name**, not position -- actually simpler and
more robust than replicating the `*Reader` classes' positional contract,
since the raw XML is itself name-keyed. `ContentLoader` has one `Load*`
method per schema (Armor/Weapon/Item/Gear-dispatch, CharacterClass, Spell,
Monster, Player, QuestNpc, Map, Chest, FixedCombat, Inn, Store,
Quest/QuestLine, GameStartDescription), each mirroring its corresponding
`*Reader.Read()`'s post-load steps (recursively loading referenced assets,
converting texture paths, running `Clone()` where the original does).
Per-type in-memory caches (`std::map<std::string, shared_ptr<T>>`) avoid
reloading/re-parsing the same asset twice, mirroring `ContentManager`'s own
caching.

**One real bug this approach surfaced and fixed:** `Item.Usage` is a
`[Flags]` C# enum. The IntermediateSerializer XML format serializes flags
enums as comma-separated symbolic names (e.g. `<Usage>Combat,NonCombat</Usage>`,
confirmed in `MinorHealingPotion.xml`), not as an integer -- unlike the
compiled `ContentTypeReader.Read()`, which reads an already-resolved
`int`. An initial `ReadInt(...)` on this field threw `std::invalid_argument`
from `std::stoi("NonCombat")` the first time a chest/quest/inventory
referencing that item was loaded. Fixed with a dedicated
`ReadItemUsage()` that parses the comma-separated flag-name list (falling
back to a bare integer for the couple of files that do use `"0"` directly).

## Asset-path case mismatches (Linux is case-sensitive)

Two classes of case mismatch surfaced only at runtime (Windows' filesystem
and content pipeline are case-insensitive; this desktop's is not), matching
this project's established precedent for this class of fix:
- **Directory name:** the shipped content ships `Characters/QuestNPCs/` but
  every C# reference (`RolePlayingGameData/Map/Map.cs`'s
  `MapReader.Read()`) uses `Characters\QuestNpcs`. Fixed by renaming the
  copied directory to `QuestNpcs` to match the code.
- **Data-driven filename references:** several XML files spell a texture
  name differently from the actual shipped PNG (e.g. `Kolatt.xml`'s
  `<InactivePortraitTextureName>Warrior1InActive</InactivePortraitTextureName>`
  vs. the real `Warrior1Inactive.png`). Rather than hand-patch every
  mismatch as it's discovered (there are 700+ textures and 281 XML files;
  this is not the only one), `ContentLoader::ResolveCaseInsensitive()` falls
  back to a case-insensitive directory scan whenever the exact-case path
  doesn't exist, for both `.xml` asset loads and texture loads. Content
  names with Windows path separators (`Items\MinorHealingPotion`) are also
  normalized to `/` in the same place.

## Audio: SoundEffect-backed, not real XACT (same precedent as every other port)

**XNA behaviour:** `AudioManager.cs` wraps a real XACT
`AudioEngine`/`SoundBank`/`WaveBank` loaded from a compiled
`RpgAudio.xgs`/`Wave Bank.xwb`/`Sound Bank.xsb` project.

**CNA port behaviour:** CNA actually implements the XACT API
(`Microsoft::Xna::Framework::Audio::AudioEngine` parses real `.xgs`/`.xwb`/
`.xsb` binaries) -- but this sample's source tree only ships the
*uncompiled* `.xap` XACT project plus raw `.wav` files, and no XACT
authoring tool is available on this desktop to compile real binaries from
it. `AudioManager.hpp` keeps the original's cue-name-based public API
(`PlayCue`/`PushMusic`/`PopMusic`/`StopMusic`) but backs it with plain
`SoundEffect`/`SoundEffectInstance`, lazily loading each cue's `.wav` by name
on first use. A missing cue (see next item) is caught and cached as "missing"
rather than crashing, mirroring the original's own
`catch (NoAudioHardwareException)` graceful-degradation.

**A real asset gap, not a porting bug:** `Map001.xml`'s `MusicCueName` is
`"BeachTheme"`, but no `BeachTheme.wav` (or any correspondingly-named cue)
ships anywhere in this sample's asset tree -- confirmed absent from the
27 `.wav` files under `Content/Audio/`. This is a real gap in the
shipped desktop mirror of this sample (the compiled `.xsb` presumably
aliased it to another wave internally); the port plays silence for this one
cue rather than crashing.

## Input: InputManager only, no InputState/touch (matches the original exactly)

Unlike every other ScreenManager port in this repo (which had to invent a
mouse/touch fallback for a Tap-gesture-driven original), this sample's
original **never had touch, mouse, or an `InputState` class at all** -- it's
Windows+Xbox only, and `HandleInput()` reads global `InputManager`
keyboard/gamepad state directly. This port keeps that shape exactly
(`GameScreen::HandleInput()` takes no parameters). CNA unifies GamePad
face/shoulder buttons and D-pad directions into one `Buttons` flags enum
(`IsButtonDown(Buttons::X)`), unlike the original's separate
`GamePadState.Buttons`/`.DPad` structs -- `InputManager::GamePadButtons`
maps onto that single CNA enum instead of mirroring the two-struct split.

## Save/load and confirmation dialogs dropped (same precedent as every other port)

No `IsolatedStorageFile`/`StorageDevice` equivalent exists in CNA. Dropped
entirely, matching every prior ScreenManager port's precedent:
- `Session.SaveSession`/`LoadSession`/`DeleteSaveGame`/`GetStorageDevice`/
  `SaveGameDescriptions` and `Party`'s `PartySaveData`-based constructor —
  not ported. `Session::StartNewSession` (the `GameStartDescription` path)
  is the only way to begin a session; it always starts fresh.
- `MenuScreens/SaveLoadScreen.cs` — not ported (its only two call sites,
  `MainMenuScreen`'s Save Game/Load Game entries, are also dropped).
- `MenuScreens/MessageBoxScreen.cs` — not ported. Its two call sites in the
  original (`GameplayScreen`'s "confirm exit" prompt,
  `SaveLoadScreen`'s overwrite confirmation) are both gone or dropped;
  `GameplayScreen`'s `ExitGame` action now exits immediately instead of
  confirming.

## Screens genuinely reachable in the original but not implemented this session (scope, not dead code)

Unlike the section above, the following **are** real, reachable features of
the original game that this port simply ran out of scope to implement in one
session -- they are not unreachable/dead in the C# the way SaveLoadScreen's
call sites are once Save/Load is dropped. Documented honestly as a scope
gap, not a deliberately-pruned dead path:
- **`MenuScreens/ControlsScreen.cs`/`HelpScreen.cs`** -- reachable via
  `MainMenuScreen`'s Controls/Help entries in the original; this port's
  `MainMenuScreen` only has New Game/Exit.
- **`GameScreens/StatisticsScreen.cs`** (character stats/equipment summary)
  -- reachable via the `CharacterManagement` action from `GameplayScreen` in
  the original; this port's `GameplayScreen::HandleInput()` only handles
  `MainMenu`/`ExitGame`.
- **`GameScreens/InventoryScreen.cs`/`EquipmentScreen.cs`/
  `SpellbookScreen.cs`** -- normally reached from `StatisticsScreen`; not
  implemented since `StatisticsScreen` itself isn't.
- **`GameScreens/PlayerSelectionScreen.cs`** (955 lines; used to pick a
  target party member for gear/item/spell actions) -- not implemented.
- **`GameScreens/StoreBuyScreen.cs`/`StoreSellScreen.cs`** -- the original's
  `StoreScreen` presents Buy/Sell as two full itemized sub-screens with
  per-category tabs and a running total. This port's `StoreScreen.hpp`
  keeps the real `BuyMultiplier`/`SellMultiplier` math but only exposes a
  single flat "Sell All Inventory" action (no buying at all yet) plus
  Leave -- a real functional gap (the party can never spend gold on new
  gear through this port), not merely a UI simplification.
- **`GameScreens/ListScreen.cs`** -- the shared generic scrollable-list base
  class several of the above build on; not ported, since none of its
  concrete users were.

## Combat simplified to a text-menu turn resolver (major simplification)

**XNA behaviour:** `Combat/CombatEngine.cs` (~2000 lines) plus
`Combatant`/`CombatantPlayer`/`CombatantMonster`/`ArtificialIntelligence`
and the `Combat/Actions/` class hierarchy (`CombatAction`,
`MeleeCombatAction`, `SpellCombatAction`, `ItemCombatAction`,
`DefendCombatAction`, ~3000 more lines) implement a real-time animated
battle stage: combatants have on-screen positions, walk/attack/dodge/hit/die
animation states driven by their `CombatSprite`, floating damage-number
"combat effects", projectile-travel timing for melee/spell/item actions, and
a full turn-order/AI state machine including fleeing, spell selection, and
item use from inventory mid-battle.

**CNA port behaviour:** `Combat/CombatEngine.hpp` (~270 lines) keeps the
same *outcome* -- turn-based combat against the same `FixedCombat`/
`RandomCombat` monster data, the same `StatisticsValue`-based damage math
(weapon `TargetDamageRange` vs. monster `PhysicalDefense`, monster
`PhysicalOffense` vs. player `PhysicalDefense`, `DefendPercentage`-based
block chance), the same gold/experience/gear-drop rewards via
`Monster::CalculateGoldReward/CalculateExperienceReward/CalculateGearDrop`,
and the same flee-probability roll -- but presents it as a plain text log
and action menu (Attack an on-screen-listed monster, cycle target with
Up/Down, Defend, or attempt to Flee) instead of the animated battle stage.
**Only Attack/Defend/Flee exist -- Spell casting and Item use in combat are
not implemented at all** (the `Combat/Actions/` hierarchy is not ported).
`Combat/Actions/` is an empty directory in this port.

## Secondary UI screens render as plain-text panels, not the original's textured chrome (major simplification)

**XNA behaviour:** Beyond combat (covered above), most of the game's "detail"
screens are richly skinned: `Hud.cs` (~700 lines, ~17 textures) draws a
scrollable character-portrait carousel with selection brackets, per-slot
active/inactive/can't-use plank backgrounds, and Y/Start button-prompt icons;
`DialogueScreen.cs` draws a wooden dialogue-box texture with wrapped body
text and Back/Select button icons; `ChestScreen.cs`/`RewardsScreen.cs` show a
scrollable icon grid of items with per-item Take/TakeAll; `StoreScreen.cs`/
`StoreBuyScreen.cs`/`StoreSellScreen.cs` show a two-pane itemized buy/sell UI;
`LevelUpScreen.cs` shows a per-stat before/after comparison grid; `GameScreens/
PopupScreen.png` and `HUD/CombatPopup.png` back the various popup frames.

**CNA port behaviour:** `Hud.hpp`, `DialogueScreen.hpp` (and everything built
on it: `ChestScreen`, `InnScreen`, `PlayerNpcScreen`, `QuestNpcScreen`,
`QuestLogScreen`, `LevelUpScreen`, `RewardsScreen`), and `StoreScreen.hpp` all
draw plain `SpriteBatch.DrawString` text (optionally over a flat translucent
rectangle) instead of loading any of the original's UI-chrome textures.
**The relevant textures are present in `Content/`** (`Content/Textures/
GameScreens/PopupScreen.png`, `Content/Textures/HUD/CombatPopup.png`,
`Content/Textures/Characters/Portraits/*.png`, `Content/Textures/Buttons/
*.png`, `Content/Textures/GameScreens/GoldIcon.png`, etc. — confirmed
converted and shipped, 720 PNGs total in this sample's `Content/`) but a
repo-wide grep of `src/GameScreens/`, `src/MenuScreens/`, and `src/Combat/`
for `Load<Texture2D>`/`Load<SpriteFont>` finds **zero** texture loads outside
`ContentLoader.hpp` (data-driven gear/item/portrait icon references, loaded
but not necessarily drawn by these screens) and `Fonts.hpp` (fonts only) —
i.e. this is a genuine "converted but unused" asset gap, not merely a
cosmetic reskin. Each affected header carries an inline comment
acknowledging the simplification and pointing at this file. Gameplay/data
logic is otherwise preserved exactly (see the per-screen notes elsewhere in
this file, e.g. Combat's damage math, Store's Buy/SellMultiplier math,
Chest's inventory transfer) — this is a presentation-layer-only gap. The
world map itself is not affected: `TileEngine.hpp` does draw real tile
textures (`map_->Texture`) and the party sprite via `AnimatingSprite`, so the
overworld view matches the original's look; only the secondary/menu screens
listed above are text-only.

**Root cause:** scope — reproducing ~17+ hand-tuned UI-chrome textures'
9-slice/scroll/carousel layout logic per screen was deprioritized against
preserving the underlying gameplay math for all of these screens in one
porting session.

**Tracked in:** not a CNA gap (no missing API — `SpriteBatch.Draw(Texture2D,
...)` works fine and is already used elsewhere, e.g. `TileEngine.hpp`); a
scope gap in this port, same class as the "Screens genuinely reachable...
not implemented" section above.

## C++ structural adaptations (no behavioral difference)

- **`Direction` member renames:** `Character.Direction`/`MapEntry<T>.Direction`
  are renamed to `CharacterDirection`/`EntryDirection` respectively --
  `RolePlayingGameData::Direction` is also a real enum type name in this
  codebase, and a member named identically to its own type risks lookup
  ambiguity (same class of fix as CardsStarterKit's `Hand`/`PlayerHand`
  rename).
- **`Gear::CheckRestrictions`** is declared in `Gear.hpp` but its body is
  defined out-of-line in `FightingCharacter.hpp`, since it needs
  `FightingCharacter` to be a complete type and `FightingCharacter` derives
  (transitively, via `Equipment`) from `Gear` -- a real circular type
  dependency, resolved the same way this project resolves any other
  cross-referencing method between mutually-dependent classes.
- **Session/CombatEngine/TileEngine/Party cross-referencing methods**:
  `Session` needs `CombatEngine`/screen classes; `CombatEngine` needs
  `Session` (rewards, game-over); `TileEngine` needs `Session`
  (encounter/random-combat checks); `Party::GiveExperience` needs
  `Session`+`LevelUpScreen`. All four classes declare the affected methods
  in their own headers but define the bodies in `RolePlayingGame.hpp`
  (the final assembly file, alongside the `Game` subclass) -- the same
  "cross-referencing method definitions" pattern already established by
  NinjAcademy/CardsStarterKit for smaller cases, just with more classes
  involved given this sample's size.
- **No garbage collector**: `ContentEntry<T>`/`MapEntry<T>`/`WorldEntry<T>`/
  `WeightedContentEntry<T>`/`QuestRequirement<T>` hold `shared_ptr<T>`
  instead of a GC-tracked reference; `Session`'s singleton is a raw
  `new`'d pointer intentionally never `delete`d except in `EndSession()`
  (which does delete it) -- matching the original's "session lives until
  explicitly ended" lifetime, not a leak.
- **`AnimatingSprite`/`Spell`/others' `Clone()`** methods return
  `shared_ptr<T>` copies instead of relying on `MemberwiseClone`/manual
  field-copy `ICloneable` implementations -- same effect, C++ idiom.

## Verification

**Build:** `RolePlayingGame_cna_samples` builds with 0 errors (confirmed via
a live `cmake --build` run, not assumed from memory).

**Interactive verification:** Genuinely mixed results, all confirmed
honestly rather than assumed:
- A clean idle-render screenshot (main menu, "New Game"/"Exit", no input
  sent before capture) is confirmed on the final build with the temporary
  debug hook below removed.
- A full New-Game → world map → quest-popup → HUD render was confirmed
  working (tile layers, party-leader sprite, "Save Mercadia" quest log
  popup, HUD showing gold/HP/MP) -- but only by temporarily adding a
  debug-only auto-trigger to `MainMenuScreen::Update()` (fire "New Game"
  after 30 frames), since `xdotool` input was intermittently not reaching
  the sample window this session (same environment flakiness documented in
  the `feedback_xdotool_shared_desktop` memory gotcha -- confirmed via
  `getactivewindow` showing correct focus while keystrokes still didn't
  register). **The debug hook was removed before this file was written; a
  deliberate, input-driven playthrough (walk into a portal/chest/NPC,
  trigger and resolve a combat) was not achieved this session** and is
  owed next time input reliably reaches sample windows.
- The crashes found and fixed via this debug-triggered path (the
  `Item.Usage` flags-enum bug, the `QuestNpcs` directory casing, the
  `Warrior1InActive` filename casing, and the missing `BeachTheme` cue) are
  real fixes verified by re-running to a clean exit past each one, in
  order -- not speculative.

## No CNA/sharp-runtime framework gaps

Every compile and runtime error encountered was a porting-side issue (wrong
CNA API usage, or a data/asset mismatch in this sample's own content) --
listed above. Nothing required a change to `cna` or `sharp-runtime`.
