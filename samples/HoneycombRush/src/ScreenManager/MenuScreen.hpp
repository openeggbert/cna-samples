#pragma once

// MenuScreen.hpp — C++ port of ScreenManager/MenuScreen.cs (XNA 4.0
// HoneycombRush sample). Every concrete subclass hardcodes fixed pixel
// positions for its entries (matching the original — its
// UpdateMenuEntryLocations auto-layout method is commented out and never
// called, so it isn't ported here either — see missing.md).

#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "System/TimeSpan.hpp"

#include "GameScreen.hpp"
#include "MenuEntry.hpp"
#include "ScreenManager.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// Base class for screens that contain a menu of options. Port of
// ScreenManager/MenuScreen.cs.
class MenuScreen : public GameScreen {
public:
    explicit MenuScreen(std::string menuTitle) : menuTitle_(std::move(menuTitle)) {
        setEnabledGestures(GestureType::Tap);

        setTransitionOnTime(System::TimeSpan::FromSeconds(0.5));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.5));
    }

    std::vector<std::shared_ptr<MenuEntry>>& MenuEntries() { return menuEntries_; }

    void HandleInput(GameTime& gameTime, InputState& input) override {
        (void)gameTime;

        PlayerIndex player;
        if (input.IsNewButtonPress(Buttons::Back, ControllingPlayer(), player)) {
            OnCancel(player);
        }

        for (const GestureSample& gesture : input.Gestures) {
            if (gesture.getGestureTypeProperty() != GestureType::Tap) continue;

            Point tapLocation((int)gesture.getPositionProperty().X, (int)gesture.getPositionProperty().Y);

            for (std::size_t i = 0; i < menuEntries_.size(); i++) {
                if (GetMenuEntryHitBounds(*menuEntries_[i]).Contains(tapLocation)) {
                    OnSelectEntry((int)i, PlayerIndex::One);
                }
            }
        }
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

        for (std::size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Update(*this, isSelected, gameTime);
        }
    }

    void Draw(const GameTime& gameTime) override {
        auto& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();

        for (std::size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Draw(*this, isSelected, gameTime);
        }

        spriteBatch.End();
    }

protected:
    // Allows the screen to create the hit bounds for a particular menu entry.
    virtual Rectangle GetMenuEntryHitBounds(MenuEntry& entry) { return entry.Bounds(); }

    // Handler for when the user has chosen a menu entry.
    virtual void OnSelectEntry(int entryIndex, PlayerIndex playerIndex) {
        menuEntries_[entryIndex]->OnSelectEntry(playerIndex);
    }

    // Handler for when the user has cancelled the menu.
    virtual void OnCancel(PlayerIndex playerIndex) {
        (void)playerIndex;
        ExitScreen();
    }

    std::vector<std::shared_ptr<MenuEntry>> menuEntries_;
    int selectedEntry_ = 0;
    std::string menuTitle_;
};

// ---- MenuEntry methods that depend on MenuScreen (defined here) ----

inline Vector2 MenuEntry::getTextPosition(MenuScreen& screen) {
    int width = GetWidth(screen);
    int height = GetHeight(screen);
    int texWidth = buttonTexture_ ? buttonTexture_->getWidthProperty() : 0;

    if (Scale == 1.0f) {
        return Vector2((float)((int)position_.X + texWidth / 2 - width / 2), (float)(int)position_.Y);
    }
    return Vector2((float)((int)position_.X + (texWidth / 2 - (int)((float)(width / 2) * Scale))),
                   (float)((int)position_.Y + (int)((float)(height - (float)height * Scale) / 2.0f)));
}

inline void MenuEntry::Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime) {
    (void)isSelected;
    (void)gameTime;

    // The original is a Windows Phone game, where MenuEntry.Draw's
    // `#if WINDOWS_PHONE` branch always forces these exact values (no
    // gamepad-style highlight state applies to a touch-only target).
    Color textColor = Color::Black;
    Color tintColor = Color::White;

    ScreenManager* screenManager = screen.GetScreenManager();
    SpriteBatch& spriteBatch = screenManager->getSpriteBatch();
    SpriteFont& font = screenManager->getFont();
    buttonTexture_ = &screenManager->getButtonBackground();

    spriteBatch.Draw(*buttonTexture_, Vector2((float)(int)position_.X, (float)(int)position_.Y), tintColor);

    spriteBatch.DrawString(font, text_, getTextPosition(screen), textColor, Rotation, Vector2::Zero, Scale,
                            SpriteEffects::None, 0.0f);
}

inline int MenuEntry::GetHeight(MenuScreen& screen) {
    return (int)screen.GetScreenManager()->getFont().MeasureString(text_).Y;
}

inline int MenuEntry::GetWidth(MenuScreen& screen) {
    return (int)screen.GetScreenManager()->getFont().MeasureString(text_).X;
}

} // namespace HoneycombRush
