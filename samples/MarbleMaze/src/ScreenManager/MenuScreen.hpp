#pragma once

// MenuScreen.hpp — C++ port of ScreenManager/MenuScreen.cs (XNA 4.0 MarbleMaze
// sample, vanilla "GameStateManagement" library). Unlike HoneycombRush's copy of
// this same library in this repo (which hardcodes fixed pixel positions and
// skips the dynamic layout), this sample's original *does* call
// UpdateMenuEntryLocations() every Draw(), auto-centering entries vertically
// starting at y=175 -- ported faithfully here.

#include <cmath>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "System/TimeSpan.hpp"

#include "GameScreen.hpp"
#include "MenuEntry.hpp"
#include "ScreenManager.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
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

    void HandleInput(InputState& input) override {
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
        UpdateMenuEntryLocations();

        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();
        SpriteFont& font = GetScreenManager()->getFont();

        spriteBatch.Begin();

        for (std::size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Draw(*this, isSelected, gameTime);
        }

        float transitionOffset = std::pow(TransitionPosition(), 2.0f);

        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        Vector2 titlePosition((float)(viewport.getWidthProperty() / 2), 80.0f);
        Vector2 titleOrigin = font.MeasureString(menuTitle_) / 2.0f;
        Color titleColor = mul(Color(192, 192, 192), TransitionAlpha());
        float titleScale = 1.25f;

        titlePosition.Y -= transitionOffset * 100.0f;

        spriteBatch.DrawString(font, menuTitle_, titlePosition, titleColor, 0.0f, titleOrigin, titleScale,
                                SpriteEffects::None, 0.0f);

        spriteBatch.End();
    }

protected:
    // Allows the screen to create the hit bounds for a particular menu entry.
    // The hit bounds are the entire width of the screen, and the height of the
    // entry with some additional padding above and below.
    virtual Rectangle GetMenuEntryHitBounds(MenuEntry& entry) {
        return Rectangle(0, (int)entry.Position().Y - MenuEntryPadding,
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

    // Allows the screen the chance to position the menu entries. By default all
    // menu entries are lined up in a vertical list, centered on the screen.
    virtual void UpdateMenuEntryLocations() {
        float transitionOffset = std::pow(TransitionPosition(), 2.0f);

        Vector2 position(0.0f, 175.0f);

        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        for (std::size_t i = 0; i < menuEntries_.size(); i++) {
            MenuEntry& menuEntry = *menuEntries_[i];

            position.X = (float)(viewport.getWidthProperty() / 2 - menuEntry.GetWidth(*this) / 2);

            if (GetScreenState() == ScreenState::TransitionOn)
                position.X -= transitionOffset * 256;
            else
                position.X += transitionOffset * 512;

            menuEntry.setPosition(position);

            position.Y += (float)(menuEntry.GetHeight(*this) + (MenuEntryPadding * 2));
        }
    }

    static constexpr int MenuEntryPadding = 35;

    std::vector<std::shared_ptr<MenuEntry>> menuEntries_;
    int selectedEntry_ = 0;
    std::string menuTitle_;
};

// ---- MenuEntry methods that depend on MenuScreen (defined here) ----

inline void MenuEntry::Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime) {
    (void)gameTime;

    Color color = isSelected ? Color::White : Color::Yellow;

    double time = gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();
    float pulsate = (float)std::sin(time * 6) + 1.0f;
    float scale = 1.0f + pulsate * 0.05f * selectionFade_;

    color = mul(Color(color.getRProperty(), color.getGProperty(), color.getBProperty()), screen.TransitionAlpha());

    ScreenManager* screenManager = screen.GetScreenManager();
    SpriteBatch& spriteBatch = screenManager->getSpriteBatch();
    SpriteFont& font = screenManager->getFont();

    Vector2 origin(0.0f, (float)(font.getLineSpacingProperty() / 2));

    spriteBatch.DrawString(font, text_, position_, color, 0.0f, origin, scale, SpriteEffects::None, 0.0f);
}

inline int MenuEntry::GetHeight(MenuScreen& screen) { return screen.GetScreenManager()->getFont().getLineSpacingProperty(); }

inline int MenuEntry::GetWidth(MenuScreen& screen) {
    return (int)screen.GetScreenManager()->getFont().MeasureString(text_).X;
}

} // namespace MarbleMazeSample
