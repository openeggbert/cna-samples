# Missing / Differences from XNA 4.0 original

## Adapted: Page 2/3 menu trees are hand-built in C++, not loaded from XML at runtime
**XNA behaviour:** Page 2 and Page 3's control trees are authored as XML content
(`Menus\MenuPage2.xml`, `Menus\MenuPage3.xml`) and loaded at runtime with
`Content.Load<Container>(...)`, which the XNA content pipeline's
`IntermediateSerializer` deserializes via reflection (using each `Control` subclass's
`[ContentSerializer]`-annotated properties) into a live `Container` object graph.
**CNA port behaviour:** CNA has no content pipeline / reflection-based XML
deserializer. `DynamicMenuGame::BuildMenuPage2Content()` and
`BuildMenuPage3Content()` construct the identical control tree directly in C++ --
same control types, same `Left`/`Top`/`Width`/`Height`/`Name`/`BackTextureName`/
`Text`/`FontName`/etc. values as the two XML files -- and `LoadControls()` still
calls `Initialize()` then `LoadContent()` on the result exactly as the original
does after its `Content.Load<Container>` call. The visible result (and the
`FindControlByName("AdvanceButton")`/`FindControlByName("ProgressBar")` lookups
Page 3 depends on) is unchanged.
**Root cause:** No XNA-style content-pipeline XML deserialization in CNA; converting
this one sample's two small pages doesn't justify writing a general-purpose
reflection-based XML loader.
**Tracked in:** Not planned -- same "convert once, not at runtime" spirit as this
project's resx-to-static-table (LocalizationSample) and font/texture asset
conversions, just applied to a small amount of layout data instead of art.

## Simplified: Control/IControl merged into a single base class
**XNA behaviour:** `IControl` is a separate interface implemented by the abstract
`Control` base class.
**CNA port behaviour:** Nothing in this sample implements `IControl` independently
of `Control` (every concrete control derives from `Control`), and C++ has no
"properties" mechanism for an interface to require the way C#'s `int Width { get;
set; }` does -- so the split serves no purpose here. `Control.hpp` merges both
into one class; likewise `TextControl` absorbs `ITextControl`.
**Root cause:** Natural C++ language difference (no properties), and the interface
had no second implementer to justify keeping the split.
**Tracked in:** Not planned -- a straightforward, behavior-preserving simplification.

## Added: mouse fallback for every tap interaction
**XNA behaviour:** The original only reads `TouchPanel.EnabledGestures`/gesture
taps -- there is no `Mouse` usage anywhere in `DynamicMenuSample.cs`.
**CNA port behaviour:** CNA does not synthesize touch/gesture events from mouse
input, so a left-click's rising edge is turned into a synthetic
`GestureSample(GestureType::Tap, ...)` at the click position and appended to the
same `gestureList` passed to `phoneScreen_.Update()` every frame
(`DynamicMenuGame::Update()`). Every button -- page-select, Page 1's four buttons,
Page 3's Advance button -- gets mouse support for free, with **no changes to the
ported Controls library itself**; the fallback lives entirely in the Game class.
**Root cause:** No touchscreen on the development machine.
**Tracked in:** Not planned -- deliberate, documented addition, matching the
precedent set by GesturesSample/TouchThumbsticks/SnowShovel.

## Added: keyboard orientation toggle (O key)
**XNA behaviour:** `Window.OrientationChanged` fires when the phone is physically
rotated, and the handler sets `phoneScreen.CurrentOrientation =
Window.CurrentOrientation`, causing `PhoneScreen` to re-lay-out its two containers
side-by-side (landscape) or stacked (portrait).
**CNA port behaviour:** There is no physical rotation sensor on this desktop, and
`Window.OrientationChanged` was not wired up (it would never fire). Instead, `O`
toggles between portrait (480x800) and landscape (800x480): it swaps
`PreferredBackBufferWidth`/`Height`, calls `GraphicsDeviceManager::ApplyChanges()`,
and directly calls `phoneScreen_.SetCurrentOrientation(...)` -- exercising the same
`PhoneScreen` re-layout logic the original's real device-rotation path uses,
just triggered manually. This is one of the sample's two headline features (per
its own `.htm`: "this sample can be used either in landscape or in portrait
modes"), so it seemed worth keeping demonstrable rather than letting it become
dead code on a desktop with no rotation sensor.
**Root cause:** No physical device-orientation sensor on this desktop.
**Tracked in:** Not planned -- deliberate, documented addition. Runtime backbuffer
resizing via `ApplyChanges()` is a comparatively lightly-exercised CNA path in this
project; if the toggle looks visually wrong, check there first before assuming a
`PhoneScreen` layout bug.

## Adapted: windowed instead of `IsFullScreen = true`
**XNA behaviour:** The constructor sets `graphics.IsFullScreen = true`
unconditionally (not `#if WINDOWS_PHONE`-gated like most other samples' phone-only
settings) -- on Windows Phone/Xbox this just fills the screen; on a desktop
Windows build it would force an actual fullscreen window.
**CNA port behaviour:** Left windowed, matching every other sample in this repo --
forcing fullscreen would make screenshotting/testing this one sample
inconsistent with the rest of the project for no behavioral benefit on desktop.
**Root cause:** Desktop dev-loop practicality; matches established repo-wide
precedent (no other sample forces fullscreen either).
**Tracked in:** Not planned.

## Verification note
Interactively confirmed by screenshot, across two sessions: Page 1 renders
correctly (page-select buttons with the active page highlighted yellow,
checkerboard background, all four buttons -- Change Hue/Index/Bounce/Get big --
correctly colored, sized, and centered-text). A follow-up session confirmed the
rest by screenshot too: Page 2 (UFO image + `MultilineTextControl` text render
correctly), Page 3 (the `Advance` button correctly increments the progress
bar's filled-arrow region in steps of 10, observed 0→1→2→3→4 arrows across
repeated clicks), and the `O` orientation toggle (portrait 480x800 ↔ landscape
800x480, round-tripped twice with no crash and the progress bar state
preserved across the resize). No bugs found in this pass beyond the one
correctness issue already caught by static review (see below).

One correctness issue *was* caught and fixed during this static review, not by
live testing: the original's `Control.Update` copies its active-transitions list
before iterating specifically because `Transition` is a C# reference type, so a
`TransitionComplete` handler calling `ApplyTransition` (see the Get-Big button,
which re-triggers a second transition from its own completion callback) safely
mutates the *real* list without corrupting the in-progress iteration. A first
draft of the C++ port stored `Transition` by value in `std::vector<Transition>`,
which would have left dangling/invalidated references the moment a
`TransitionComplete` callback added a new transition during iteration. Fixed by
storing `std::shared_ptr<Transition>` instead, copying the list of *pointers*
(cheap, and safe against reallocation) before iterating -- see the comment in
`Control::Update`.

## RNG lifecycle: persistent `System::Random` instead of a fresh `Random` per tap
**XNA behaviour:** `hueChangeButton_Tapped` constructs `Random rnd = new Random();`
-- fresh, time-seeded, and discarded -- on every single tap. .NET's
parameterless `Random()` seeds from `Environment.TickCount`, so two taps
within the same clock tick can even reproduce the identical "random" color, a
known quirk of this construct-per-call pattern.
**CNA port behaviour:** `DynamicMenuGame` holds one `System::Random random_;`
member, seeded once at construction, and `HueChangeButtonTapped` draws from
that single persistent generator on every tap instead of constructing a new
one. The color formula itself (r/g/b/a via `NextDouble()`, complementary font
color, `Color(r,g,b)*a`) is byte-for-byte identical -- only the generator's
instantiation/reseed lifecycle differs.
**Root cause:** Idiomatic C++ RNG usage (see CLAUDE.md's `System::Random
random;` member guidance) instead of literally reproducing the original's
unusual "new Random() per call" pattern.
**Tracked in:** Not planned -- a strictly-better RNG usage pattern with no
visible behavioral downside (still produces a new-looking random color per
tap).

## Added: Escape key also exits the game
**XNA behaviour:** `Update()` exits only on
`GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed` -- no
keyboard exit path exists anywhere in this Windows-Phone-only original.
**CNA port behaviour:** `Update()` exits on GamePad Back **or**
`Keyboard::GetState().IsKeyDown(Keys::Escape)`, for keyboard-only desktop
testing.
**Root cause:** Desktop dev-loop practicality -- no gamepad guaranteed
attached.
**Tracked in:** Not planned -- minor, deliberate desktop-usability addition,
same class as `samples/SoccerPitch/missing.md`'s equivalent note.

## No known differences beyond the above
Page 1's four buttons (hue randomization + complementary font color -- see the
RNG lifecycle note above for the one caveat, tap-index counter, bounce
animation state machine, get-big-then-shrink-back transition chain), the
Transition system's position/size/color interpolation and the four
`CreateFadeIn`/`CreateFadeOut`/`CreateFlyIn`/`CreateFlyOut` factory helpers
(present in the library, unused by this sample -- matching the original, which
doesn't call them either), the Button press-and-hold-briefly-then-fire timing,
MultilineTextControl's word-wrap algorithm, and ProgressBar's left/right split
rendering are a direct, faithful port of the DynamicMenu library and
`DynamicMenuSample.cs`. The `Fonts\ControlFont` "Segoe UI Mono" substitution
(DejaVu Sans Mono) follows this project's established font-substitution
convention.
