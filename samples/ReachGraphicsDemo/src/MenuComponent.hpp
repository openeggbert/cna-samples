#pragma once

// Ported from XnaGraphicsDemo.MenuComponent (MenuComponent.cs). Base class for all the
// different screens used in the demo. Provides a simple touch (here: mouse) menu which
// can display a list of options and detect when a menu item is clicked.
//
// DemoGame is only forward-declared here -- MenuComponent's own methods that need
// DemoGame's members (Update, Draw, DrawTitle, GetSpriteBatch/GetFont/GetBigFont) are
// declared here but DEFINED out-of-line at the bottom of DemoGame.hpp, once DemoGame is
// a complete type. This mirrors the exact header-split already established by this
// repo's GameStateManagement port (GameScreen.hpp forward-declares ScreenManager the
// same way, with GameScreen::Update/ExitScreen defined out-of-line in
// ScreenManager.hpp).

#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "System/TimeSpan.hpp"

#include "MenuEntry.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using System::TimeSpan;

class DemoGame; // forward declaration -- see comment above

class MenuComponent : public DrawableGameComponent {
public:
    // Defined out-of-line at the bottom of DemoGame.hpp (the base DrawableGameComponent
    // constructor needs a Game&, which requires DemoGame's inheritance from Game to be
    // visible -- i.e. DemoGame must be a complete type).
    explicit MenuComponent(DemoGame& game);

    const std::string& GetTypeName() const override {
        static const std::string name = "MenuComponent";
        return name;
    }

    // Initializes the menu, computing the screen position of each entry. Self-contained
    // (only needs MenuEntry::Height/Border), so this can stay inline.
    void Initialize() override {
        Vector2 pos(static_cast<float>(MenuEntry::Border),
                    800.0f - MenuEntry::Border - static_cast<float>(Entries.size()) * MenuEntry::Height);

        for (auto& entry : Entries) {
            entry->Position = pos;
            pos.Y += MenuEntry::Height;
        }

        DrawableGameComponent::Initialize();
    }

    // Resets the menu, whenever we transition to or from a different screen.
    virtual void Reset() {
        if (touchSelection_ >= 0) {
            Entries[static_cast<std::size_t>(touchSelection_)]->IsFocused = false;
        }

        touchDown_ = true;
        touchSelection_ = -1;
    }

    // Updates the menu state, processing user input. Defined out-of-line (needs
    // DemoGame::Graphics/IsActive).
    void Update(GameTime& gameTime) override;

    // Draws the list of menu entries. Defined out-of-line (needs
    // GetSpriteBatch()/GetFont()/DemoGame::ScaleMatrix).
    void Draw(const GameTime& gameTime) override;

protected:
    std::vector<std::unique_ptr<MenuEntry>> Entries;
    Vector2 LastTouchPoint;

    // Draws the menu title. Defined out-of-line (needs
    // GetSpriteBatch()/GetBigFont()/DemoGame::ScaleMatrix).
    void DrawTitle(const std::string& title, std::optional<Color> backgroundColor, Color titleColor);

    // Allows subclasses to customize their attract behavior. The default is to simulate
    // a click on the last menu entry, which is usually "back".
    virtual void OnAttract() { Entries.back()->OnClicked(); }

    // Allows subclasses to customize how long they wait before cycling through the
    // attract sequence.
    virtual TimeSpan AttractDelay() const { return TimeSpan::FromSeconds(10); }

    // Handles a drag on the background of the screen.
    virtual void OnDrag(Vector2 /*delta*/) {}

    // Convenience accessors mirroring the C# original's `Game`-cast properties. Defined
    // out-of-line (need DemoGame to be complete).
    DemoGame& GetGame() const { return game_; }
    SpriteBatch& GetSpriteBatch() const;
    SpriteFont& GetFont() const;
    SpriteFont& GetBigFont() const;

private:
    // Handles input while a touch is occurring.
    void HandleTouchDown(int touchX, int touchY) {
        // Hit test the touch position against the list of menu items.
        int currentEntry = -1;

        for (std::size_t i = 0; i < Entries.size(); ++i) {
            if (touchY >= Entries[i]->Position.Y && touchY < Entries[i]->Position.Y + MenuEntry::Height) {
                currentEntry = static_cast<int>(i);
                break;
            }
        }

        if (touchDown_) {
            // Are we already processing a touch?
            if (touchSelection_ >= 0) {
                auto& sel = Entries[static_cast<std::size_t>(touchSelection_)];
                if (currentEntry == touchSelection_ || sel->IsDraggable) {
                    // Pass drag input to the currently selected item.
                    sel->IsFocused = true;
                    sel->OnDragged(touchX - LastTouchPoint.X);
                } else {
                    // If the drag moves off the selected item, unfocus it.
                    sel->IsFocused = false;
                }
            } else {
                // If the touch was not on any menu item, process a background drag.
                OnDrag(Vector2(static_cast<float>(touchX), static_cast<float>(touchY)) - LastTouchPoint);
            }
        } else {
            // We are not currently processing a touch.
            touchDown_ = true;
            touchSelection_ = currentEntry;

            if (touchSelection_ >= 0) {
                // Focus the menu item that has just been touched.
                Entries[static_cast<std::size_t>(touchSelection_)]->IsFocused = true;
            }
        }

        // Store the most recent touch location.
        LastTouchPoint = Vector2(static_cast<float>(touchX), static_cast<float>(touchY));
    }

    // Handles input when the touch is released.
    void HandleTouchUp() {
        if (touchDown_ && touchSelection_ >= 0 && Entries[static_cast<std::size_t>(touchSelection_)]->IsFocused) {
            // If we were touching a menu item, and just released it, process the click action.
            Entries[static_cast<std::size_t>(touchSelection_)]->IsFocused = false;
            Entries[static_cast<std::size_t>(touchSelection_)]->OnClicked();
        }

        touchDown_ = false;
        touchSelection_ = -1;
    }

    // If no input is provided, we go into an automatic attract mode, which cycles
    // through the various options. This was great for leaving the demo unattended at
    // the kiosk during the MIX10 conference!
    void HandleAttractMode(const GameTime& gameTime, const MouseState& input) {
        if (input != lastInputState_ || touchDown_) {
            // If input has changed, reset the timer.
            attractTimer_ = TimeSpan::FromSeconds(-15);
            lastInputState_ = input;
        } else {
            // If no input occurs, increment the timer.
            attractTimer_ = attractTimer_ + gameTime.getElapsedGameTimeProperty();

            if (attractTimer_ > AttractDelay()) {
                // Timeout! Run the attract action.
                attractTimer_ = TimeSpan::Zero;
                OnAttract();
            }
        }
    }

    DemoGame& game_;

    bool touchDown_ = true;
    int touchSelection_ = -1;

    // NOXNA note: the C# original declares `attractTimer`/`lastInputState` as STATIC
    // fields on MenuComponent, shared across every menu screen instance (not per-screen)
    // -- since only the currently-active screen's Update() ever runs (every other
    // screen has Enabled=false), this acts as a single global attract-mode timer that
    // persists correctly across screen switches. Reproduced with C++17 inline statics
    // (no separate .cpp definition needed) to match that exact sharing behaviour.
    inline static TimeSpan attractTimer_;
    inline static MouseState lastInputState_ = MouseState(-1, -1, -1, ButtonState::Released, ButtonState::Released,
                                                            ButtonState::Released, ButtonState::Released,
                                                            ButtonState::Released);
};

} // namespace ReachGraphicsDemoSample
