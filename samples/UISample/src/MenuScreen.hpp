#pragma once

#include <cmath>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "System/TimeSpan.hpp"

#include "GameScreen.hpp"
#include "MenuEntry.hpp"
#include "ScreenManager.hpp"

namespace UISample {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// Base class for screens that contain a menu of options. The user taps an
// entry to select it, or presses Back to cancel out of the screen. Port of
// Screens/MenuScreen.cs.
class MenuScreen : public GameScreen {
public:
    explicit MenuScreen(std::string menuTitle) : menuTitle_(std::move(menuTitle)) {
        // menus generally only need Tap for menu selection
        setEnabledGestures(GestureType::Tap);

        setTransitionOnTime(System::TimeSpan::FromSeconds(0.5));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.5));
    }

    std::vector<std::shared_ptr<MenuEntry>>& MenuEntries() { return menuEntries_; }

    void HandleInput(InputState& input) override {
        // we cancel the current menu screen if the user presses the back button
        PlayerIndex player;
        if (input.IsNewButtonPress(Buttons::Back, ControllingPlayer(), player)) {
            OnCancel(player);
        }

        // look for any taps that occurred and select any entries that were tapped
        for (const GestureSample& gesture : input.Gestures) {
            if (gesture.getGestureTypeProperty() != GestureType::Tap) continue;

            Point tapLocation((int)gesture.getPositionProperty().X, (int)gesture.getPositionProperty().Y);

            for (std::size_t i = 0; i < menuEntries_.size(); i++) {
                if (GetMenuEntryHitBounds(*menuEntries_[i]).Contains(tapLocation)) {
                    // select the entry. since gestures are only available via
                    // touch/mouse fallback, we can safely pass PlayerIndex::One
                    // to all entries since there is only one local player.
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
        // make sure our entries are in the right place before we draw them
        UpdateMenuEntryLocations();

        auto& graphics = GetScreenManager()->getGraphicsDeviceProperty();
        auto& spriteBatch = GetScreenManager()->getSpriteBatch();
        auto& font = GetScreenManager()->getFont();

        spriteBatch.Begin();

        for (std::size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Draw(*this, isSelected, gameTime);
        }

        // Make the menu slide into place during transitions, using a power
        // curve to make things look more interesting (this makes the
        // movement slow down as it nears the end).
        float transitionOffset = std::pow(TransitionPosition(), 2.0f);

        // Draw the menu title centered on the screen
        Vector2 titlePosition((float)(graphics.getViewportProperty().getWidthProperty() / 2), 80.0f);
        Vector2 titleOrigin = font.MeasureString(menuTitle_) / 2.0f;
        Color titleColor = mul(Color(192, 192, 192), TransitionAlpha());
        float titleScale = 1.25f;

        titlePosition.Y -= transitionOffset * 100.0f;

        spriteBatch.DrawString(font, menuTitle_, titlePosition, titleColor, 0.0f, titleOrigin,
                               titleScale, SpriteEffects::None, 0.0f);

        spriteBatch.End();
    }

protected:
    // Allows the screen to create the hit bounds for a particular menu entry.
    virtual Rectangle GetMenuEntryHitBounds(MenuEntry& entry) {
        // the hit bounds are the entire width of the screen, and the height
        // of the entry with some additional padding above and below.
        return Rectangle(
            0,
            (int)entry.Position().Y - MenuEntryPadding,
            GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getWidthProperty(),
            entry.GetHeight(*this) + (MenuEntryPadding * 2));
    }

    // Handler for when the user has chosen a menu entry.
    virtual void OnSelectEntry(int entryIndex, PlayerIndex playerIndex) {
        menuEntries_[entryIndex]->OnSelectEntry(playerIndex);
    }

    // Handler for when the user has cancelled the menu.
    virtual void OnCancel(PlayerIndex playerIndex) {
        (void)playerIndex;
        ExitScreen();
    }

    // Allows the screen the chance to position the menu entries. By default
    // all menu entries are lined up in a vertical list, centered on the screen.
    virtual void UpdateMenuEntryLocations() {
        float transitionOffset = std::pow(TransitionPosition(), 2.0f);

        // start at Y = 175; each X value is generated per entry
        Vector2 position(0.0f, 175.0f);

        for (auto& menuEntry : menuEntries_) {
            int viewportWidth = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getWidthProperty();
            position.X = (float)(viewportWidth / 2 - menuEntry->GetWidth(*this) / 2);

            if (GetScreenState() == ScreenState::TransitionOn)
                position.X -= transitionOffset * 256.0f;
            else
                position.X += transitionOffset * 512.0f;

            menuEntry->setPosition(position);

            position.Y += (float)(menuEntry->GetHeight(*this) + (MenuEntryPadding * 2));
        }
    }

    // the number of pixels to pad above and below menu entries for touch input
    static constexpr int MenuEntryPadding = 10;

    std::vector<std::shared_ptr<MenuEntry>> menuEntries_;
    int selectedEntry_ = 0;
    std::string menuTitle_;
};

// ---- MenuEntry methods that depend on MenuScreen (defined here) ----

inline void MenuEntry::Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime) {
    // there is no such thing as a selected item on this touch-only sample
    isSelected = false;

    Color color = isSelected ? Color::Yellow : Color::White;

    // Pulsate the size of the selected menu entry.
    double time = gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();
    float pulsate = (float)std::sin(time * 6.0) + 1.0f;
    float scale = 1.0f + pulsate * 0.05f * selectionFade_;

    // Modify the alpha to fade text out during transitions.
    color = mul(Color(color.getRProperty(), color.getGProperty(), color.getBProperty()),
                screen.TransitionAlpha());

    auto& spriteBatch = screen.GetScreenManager()->getSpriteBatch();
    auto& font = screen.GetScreenManager()->getFont();

    Vector2 origin(0.0f, (float)font.getLineSpacingProperty() / 2.0f);

    spriteBatch.DrawString(font, text_, position_, color, 0.0f, origin, scale,
                           SpriteEffects::None, 0.0f);
}

inline int MenuEntry::GetHeight(MenuScreen& screen) {
    return screen.GetScreenManager()->getFont().getLineSpacingProperty();
}

inline int MenuEntry::GetWidth(MenuScreen& screen) {
    return (int)screen.GetScreenManager()->getFont().MeasureString(text_).X;
}

} // namespace UISample
