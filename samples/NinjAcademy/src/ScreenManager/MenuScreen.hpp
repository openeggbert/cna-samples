#pragma once

// MenuScreen.hpp — C++ port of ScreenManager/MenuScreen.cs (XNA 4.0
// NinjAcademy sample). Base class for screens that contain a menu of
// options. Unlike the stock XNA "Game State Management" template, this
// sample's MenuScreen has no keyboard up/down navigation -- entries are
// selected purely by tapping them (mouse click via the InputState fallback,
// see InputState.hpp), matching the original exactly.

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"

#include "ScreenManager.hpp"
#include "MenuEntry.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;

// Base class for screens that contain a menu of options. Port of
// ScreenManager/MenuScreen.cs.
class MenuScreen : public GameScreen {
public:
    explicit MenuScreen(const std::string& menuTitle) : menuTitle_(menuTitle) {
        // Menus generally only need Tap for menu selection.
        setEnabledGestures(GestureType::Tap);

        setTransitionOnTime(TimeSpan::FromSeconds(0.5));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    // Responds to user input, changing the selected entry and accepting or
    // cancelling the menu.
    void HandleInput(InputState& input) override {
        // Cancel the current menu screen if the user presses the back button.
        PlayerIndex player;
        if (input.IsNewButtonPress(Buttons::Back, ControllingPlayer(), player)) {
            OnCancel(player);
        }

        // Look for any taps that occurred and select any entries that were tapped.
        for (auto& gesture : input.Gestures) {
            if (gesture.getGestureTypeProperty() != GestureType::Tap)
                continue;

            Point tapLocation((int)gesture.getPositionProperty().X, (int)gesture.getPositionProperty().Y);

            for (size_t i = 0; i < menuEntries_.size(); i++) {
                if (GetMenuEntryHitBounds(*menuEntries_[i]).Contains(tapLocation)) {
                    OnSelectEntry((int)i, PlayerIndex::One);
                }
            }
        }
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

        for (size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Update(*this, isSelected, gameTime);
        }
    }

    void Draw(const GameTime& gameTime) override {
        UpdateMenuEntryLocations();

        auto& graphics = screenManager_->getGraphicsDeviceProperty();
        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
        SpriteFont& font = screenManager_->getFont();

        spriteBatch.Begin();

        for (size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Draw(*this, isSelected, gameTime);
        }

        float transitionOffset = (float)std::pow(TransitionPosition(), 2);

        Vector2 titlePosition((float)(graphics.getViewportProperty().getWidthProperty() / 2), 80.0f);
        Vector2 titleOrigin = font.MeasureString(menuTitle_) / 2.0f;
        Color titleColor = mul(Color(192, 192, 192), TransitionAlpha());
        float titleScale = 1.25f;

        titlePosition.Y -= transitionOffset * 100;

        spriteBatch.DrawString(font, menuTitle_, titlePosition, titleColor, 0.0f, titleOrigin, titleScale,
                               SpriteEffects::None, 0.0f);

        spriteBatch.End();
    }

protected:
    std::vector<std::shared_ptr<MenuEntry>>& MenuEntries() { return menuEntries_; }

    // Allows the screen to create the hit bounds for a particular menu entry:
    // the entire width of the screen, and the height of the entry with some
    // additional padding above and below.
    virtual Rectangle GetMenuEntryHitBounds(MenuEntry& entry) {
        return Rectangle(0, (int)entry.Position().Y - MenuEntryPadding,
                          screenManager_->getGraphicsDeviceProperty().getViewportProperty().getWidthProperty(),
                          entry.GetHeight(*this) + (MenuEntryPadding * 2));
    }

    virtual void OnSelectEntry(int entryIndex, PlayerIndex playerIndex) {
        menuEntries_[entryIndex]->OnSelectEntry(playerIndex);
    }

    virtual void OnCancel(PlayerIndex playerIndex) {
        (void)playerIndex;
        ExitScreen();
    }

    // Positions the menu entries. By default all entries are lined up in a
    // vertical list, centered on the screen.
    virtual void UpdateMenuEntryLocations() {
        float transitionOffset = (float)std::pow(TransitionPosition(), 2);

        Vector2 position(0.0f, 175.0f);

        for (size_t i = 0; i < menuEntries_.size(); i++) {
            auto& menuEntry = menuEntries_[i];

            position.X = (float)(screenManager_->getGraphicsDeviceProperty().getViewportProperty().getWidthProperty() /
                                  2) -
                         menuEntry->GetWidth(*this) / 2.0f;

            if (GetScreenState() == ScreenState::TransitionOn)
                position.X -= transitionOffset * 256;
            else
                position.X += transitionOffset * 512;

            menuEntry->setPosition(position);
            position.Y += menuEntry->GetHeight(*this) + (MenuEntryPadding * 2);
        }
    }

    static const int MenuEntryPadding = 5;

private:
    std::vector<std::shared_ptr<MenuEntry>> menuEntries_;
    int selectedEntry_ = 0;
    std::string menuTitle_;
};

// ---- MenuEntry methods that depend on MenuScreen / ScreenManager ----

inline void MenuEntry::Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime) {
    // Draw the selected entry in gray, otherwise white.
    Color color = isSelected ? Color::Gray : Color::White;
    Color shadowColor = Color::Black;

    // Pulsate the size of the selected menu entry.
    double time = gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();
    float pulsate = (float)std::sin(time * 6) + 1;
    float scale = 1 + pulsate * 0.05f * selectionFade_;

    // Modify the alpha to fade text out during transitions.
    color = mul(color, screen.TransitionAlpha());

    ScreenManager* sm = screen.GetScreenManager();
    SpriteBatch& spriteBatch = sm->getSpriteBatch();
    SpriteFont& font = sm->getFont();

    spriteBatch.DrawString(font, Text(), Position() + Vector2(4, 4), shadowColor, 0.0f, Vector2::Zero, scale,
                           SpriteEffects::None, 0.0f);
    spriteBatch.DrawString(font, Text(), Position(), color, 0.0f, Vector2::Zero, scale, SpriteEffects::None, 0.0f);
}

inline int MenuEntry::GetHeight(MenuScreen& screen) {
    return screen.GetScreenManager()->getFont().getLineSpacingProperty();
}

inline int MenuEntry::GetWidth(MenuScreen& screen) {
    return (int)screen.GetScreenManager()->getFont().MeasureString(Text()).X;
}

} // namespace NinjAcademy
