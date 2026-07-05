#pragma once

// MenuScreen.hpp -- C++ port of ScreenManager/MenuScreen.cs (XNA 4.0
// CardsStarterKit sample). Unlike the stock "Game State Management" template,
// this sample's menu entries are laid out in a horizontal row across the
// bottom of the screen (UpdateMenuEntryDestination()) and selected by mouse
// press-then-release-within-bounds (matching Blackjack/UI/Button.cs's own
// click model), not by a single tap. The XBOX/WINDOWS_PHONE branches of the
// original (gamepad-only entry list, touch gestures) are dropped -- this
// port targets desktop/mouse+keyboard only, same precedent as every other
// ScreenManager port in this repo. See missing.md.

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"

#include "ScreenManager.hpp"
#include "MenuEntry.hpp"

namespace GameStateManagement {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::ButtonState;

// Base class for screens that contain a menu of options.
class MenuScreen : public GameScreen {
public:
    explicit MenuScreen(const std::string& menuTitle) : menuTitle_(menuTitle) {
        setTransitionOnTime(TimeSpan::FromSeconds(0.5));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void HandleInput(InputState& input) override {
        PlayerIndex player;
        if (input.IsNewButtonPress(Buttons::Back, ControllingPlayer(), player)) {
            OnCancel(player);
        }

        if (input.IsMenuUp(ControllingPlayer())) {
            selectedEntry_--;
            if (selectedEntry_ < 0)
                selectedEntry_ = (int)menuEntries_.size() - 1;
        } else if (input.IsMenuDown(ControllingPlayer())) {
            selectedEntry_++;
            if (selectedEntry_ >= (int)menuEntries_.size())
                selectedEntry_ = 0;
        } else if (input.IsNewKeyPress(Keys::Enter, ControllingPlayer(), player) ||
                   input.IsNewKeyPress(Keys::Space, ControllingPlayer(), player)) {
            OnSelectEntry(selectedEntry_, player);
        }

        MouseState state = Mouse::GetState();
        if (state.getLeftButtonProperty() == ButtonState::Released) {
            if (isMouseDown_) {
                isMouseDown_ = false;
                Point clickLocation(state.getXProperty(), state.getYProperty());

                for (size_t i = 0; i < menuEntries_.size(); i++) {
                    if (menuEntries_[i]->Destination.Contains(clickLocation)) {
                        // Gestures are Windows-Phone-only in the original, so it
                        // safely hardcoded PlayerIndex.One for the mouse path too.
                        OnSelectEntry((int)i, PlayerIndex::One);
                    }
                }
            }
        } else if (state.getLeftButtonProperty() == ButtonState::Pressed) {
            isMouseDown_ = true;
            Point clickLocation(state.getXProperty(), state.getYProperty());

            for (size_t i = 0; i < menuEntries_.size(); i++) {
                if (menuEntries_[i]->Destination.Contains(clickLocation))
                    selectedEntry_ = (int)i;
            }
        }
    }

    void LoadContent() override {
        GameScreen::LoadContent();
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus,
                bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

        for (size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            UpdateMenuEntryDestination();
            menuEntries_[i]->Update(*this, isSelected, gameTime);
        }
    }

    void Draw(const GameTime& gameTime) override {
        auto& graphics = screenManager_->getGraphicsDeviceProperty();
        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
        SpriteFont& font = screenManager_->getFont();

        spriteBatch.Begin();

        for (size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Draw(*this, isSelected, gameTime);
        }

        float transitionOffset = (float)std::pow(TransitionPosition(), 2);

        Vector2 titlePosition((float)(graphics.getViewportProperty().getWidthProperty() / 2), 375.0f);
        Vector2 ms = font.MeasureString(menuTitle_);
        Vector2 titleOrigin(ms.X * 0.5f, ms.Y * 0.5f);
        Color titleColor = mul(Color(192, 192, 192), TransitionAlpha());
        float titleScale = 1.25f;

        titlePosition.Y -= transitionOffset * 100;

        spriteBatch.DrawString(font, menuTitle_, titlePosition, titleColor, 0.0f,
                               titleOrigin, titleScale, SpriteEffects::None, 0.0f);

        spriteBatch.End();
    }

    // Arranges menu entries in a horizontal row across the bottom of the
    // screen, each sized to the widest entry's text plus padding, drawn over
    // ScreenManager's ButtonBackground texture.
    void UpdateMenuEntryDestination() {
        Rectangle bounds = screenManager_->SafeArea();
        Rectangle textureSize = screenManager_->getButtonBackground().getBoundsProperty();
        int xStep = bounds.Width / ((int)menuEntries_.size() + 2);

        int maxWidth = 0;
        for (auto& entry : menuEntries_) {
            int width = entry->GetWidth(*this);
            if (width > maxWidth) maxWidth = width;
        }
        maxWidth += 20;

        for (size_t i = 0; i < menuEntries_.size(); i++) {
            menuEntries_[i]->Destination = Rectangle(
                bounds.X + (xStep - textureSize.Width) / 2 + ((int)i + 1) * xStep,
                bounds.Y + bounds.Height - textureSize.Height * 2, maxWidth, 50);
        }
    }

protected:
    std::vector<std::shared_ptr<MenuEntry>>& MenuEntries() { return menuEntries_; }

    virtual void OnSelectEntry(int entryIndex, PlayerIndex playerIndex) {
        menuEntries_[entryIndex]->OnSelectEntry(playerIndex);
    }

    virtual void OnCancel(PlayerIndex playerIndex) {
        (void)playerIndex;
        ExitScreen();
    }

private:
    std::vector<std::shared_ptr<MenuEntry>> menuEntries_;
    int selectedEntry_ = 0;
    std::string menuTitle_;
    bool isMouseDown_ = false;
};

// ---- MenuEntry methods that depend on MenuScreen / ScreenManager ----

inline void MenuEntry::Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime) {
    (void)gameTime;
    Color textColor = isSelected ? Color::White : Color::Black;
    Color tintColor = isSelected ? Color::White : Color::Gray;

    ScreenManager* sm = screen.GetScreenManager();
    SpriteBatch& spriteBatch = sm->getSpriteBatch();
    SpriteFont& font = sm->getFont();

    spriteBatch.Draw(sm->getButtonBackground(), Destination, tintColor);
    spriteBatch.DrawString(font, text_, getTextPosition(screen), textColor, Rotation,
                           Vector2::Zero, Scale, SpriteEffects::None, 0.0f);
}

inline int MenuEntry::GetHeight(MenuScreen& screen) const {
    return screen.GetScreenManager()->getFont().getLineSpacingProperty();
}

inline int MenuEntry::GetWidth(MenuScreen& screen) const {
    return (int)screen.GetScreenManager()->getFont().MeasureString(text_).X;
}

inline Vector2 MenuEntry::getTextPosition(MenuScreen& screen) const {
    if (Scale == 1.0f) {
        return Vector2((float)((int)Destination.X + Destination.Width / 2 - GetWidth(screen) / 2),
                       (float)(int)Destination.Y);
    }
    return Vector2((float)((int)Destination.X + (Destination.Width / 2 - (int)(GetWidth(screen) / 2 * Scale))),
                   (float)((int)Destination.Y + (int)((GetHeight(screen) - GetHeight(screen) * Scale) / 2)));
}

} // namespace GameStateManagement
