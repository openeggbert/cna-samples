#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"

#include "ScreenManager.hpp"
#include "MenuEntry.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// Base class for screens that contain a menu of options. Ported from
// Yacht's own ScreenManager/MenuScreen.cs, which (unlike the generic
// GameStateManagement MenuScreen) hit-tests each MenuEntry's own
// Destination rectangle directly instead of auto-laying-out a vertical
// list (that auto-layout, UpdateMenuEntryDestination(), is never actually
// called anywhere in the shipped sample and is dropped here as dead code).
//
// Per the approved plan's input design, every hit-test gets two small call
// sites -- one walking input.Gestures (the original's own WINDOWS_PHONE
// branch), one reading the mouse (the original's own WINDOWS branch) --
// both funnelling into the same Destination-rectangle hit test.
class MenuScreen : public GameScreen {
public:
    explicit MenuScreen(const std::string& menuTitle) : menuTitle_(menuTitle) {
        // The original only enabled Tap on WINDOWS_PHONE; CNA's touch is
        // real on every platform, so enable it unconditionally.
        setEnabledGestures(GestureType::Tap);

        setTransitionOnTime(TimeSpan::FromSeconds(0.5));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void HandleInput(InputState& input) override {
        PlayerIndex player;
        if (input.IsNewButtonPress(Buttons::Back, ControllingPlayer(), player)) {
            OnCancel(player);
        }

        // ---- Keyboard navigation + mouse click (desktop input) ----
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

        if (input.IsNewLeftMouseRelease()) {
            if (isMouseDown_) {
                isMouseDown_ = false;
                Point clickLocation((int)input.MousePosition().X, (int)input.MousePosition().Y);
                for (size_t i = 0; i < menuEntries_.size(); i++) {
                    if (menuEntries_[i]->getDestination().Contains(clickLocation))
                        OnSelectEntry((int)i, PlayerIndex::One);
                }
            }
        } else if (input.IsLeftMouseDown()) {
            isMouseDown_ = true;
            Point clickLocation((int)input.MousePosition().X, (int)input.MousePosition().Y);
            for (size_t i = 0; i < menuEntries_.size(); i++) {
                if (menuEntries_[i]->getDestination().Contains(clickLocation))
                    selectedEntry_ = (int)i;
            }
        }

        // ---- Touch gestures (parallel to the mouse branch above) ----
        for (const auto& gesture : input.Gestures) {
            if (gesture.getGestureTypeProperty() == GestureType::Tap) {
                Point tapLocation((int)gesture.getPositionProperty().X, (int)gesture.getPositionProperty().Y);
                for (size_t i = 0; i < menuEntries_.size(); i++) {
                    if (menuEntries_[i]->getDestination().Contains(tapLocation))
                        OnSelectEntry((int)i, PlayerIndex::One);
                }
            }
        }
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus,
                bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
        for (size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Update(*this, isSelected, gameTime);
        }
    }

    void Draw(const GameTime& gameTime) override {
        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
        SpriteFont& font = screenManager_->getFont();

        spriteBatch.Begin();

        for (size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Draw(*this, isSelected, gameTime);
        }

        // menuTitle_ is always empty in every screen that actually ships in
        // Yacht (both MainMenuScreen and NewGameSubMenuScreen construct
        // this base with ""), so this is a no-op draw call in practice --
        // kept for fidelity with the original, which draws it unconditionally.
        if (!menuTitle_.empty()) {
            float transitionOffset = (float)std::pow(TransitionPosition(), 2);

            Vector2 titlePosition((float)(screenManager_->getGraphicsDeviceProperty()
                                             .getViewportProperty().getWidthProperty() / 2),
                                  375.0f);
            Vector2 measure = font.MeasureString(menuTitle_);
            Vector2 titleOrigin(measure.X / 2.0f, measure.Y / 2.0f);
            Color titleColor = mul(Color(192, 192, 192), TransitionAlpha());
            float titleScale = 1.25f;

            titlePosition.Y -= transitionOffset * 100;

            spriteBatch.DrawString(font, menuTitle_, titlePosition, titleColor, 0.0f,
                                   titleOrigin, titleScale, SpriteEffects::None, 0.0f);
        }

        spriteBatch.End();
    }

    // Access to ScreenManager font/spritebatch for MenuEntry (defined below).
    ScreenManager& getScreenManagerRef() { return *screenManager_; }

protected:
    std::vector<std::shared_ptr<MenuEntry>>& MenuEntries() { return menuEntries_; }

    virtual void OnSelectEntry(int entryIndex, PlayerIndex playerIndex) {
        menuEntries_[entryIndex]->OnSelectEntry(playerIndex);
    }

    virtual void OnCancel(PlayerIndex playerIndex) {
        (void)playerIndex;
        ExitScreen();
    }

    std::vector<std::shared_ptr<MenuEntry>> menuEntries_;
    int selectedEntry_ = 0;
    std::string menuTitle_;

private:
    bool isMouseDown_ = false;
};

// ---- MenuEntry methods that need ScreenManager/MenuScreen internals ----

inline Vector2 MenuEntry::GetTextPosition(MenuScreen& screen) {
    if (Scale == 1.0f) {
        return Vector2((float)(destination_.X + destination_.Width / 2 - GetWidth(screen) / 2),
                       (float)destination_.Y);
    }
    return Vector2((float)(destination_.X + (destination_.Width / 2 - (int)((GetWidth(screen) / 2) * Scale))),
                   (float)(destination_.Y + (int)((GetHeight(screen) - GetHeight(screen) * Scale) / 2)));
}

inline void MenuEntry::Draw(MenuScreen& screen, bool isSelected, const GameTime& /*gameTime*/) {
    // The original forces White/White whenever built for WINDOWS_PHONE
    // (there is no "selected" concept on a touch-only device); since this
    // port also supports keyboard/gamepad navigation, the non-phone
    // (WINDOWS-build) coloring is kept instead.
    Color textColor = isSelected ? Color::White : Color::Black;
    Color tintColor = isSelected ? Color::White : Color::Gray;

    ScreenManager& screenManager = screen.getScreenManagerRef();
    SpriteBatch& spriteBatch = screenManager.getSpriteBatch();
    SpriteFont& font = screenManager.getFont();

    spriteBatch.Draw(screenManager.getBlankTexture(), destination_, tintColor);

    spriteBatch.DrawString(font, text_, GetTextPosition(screen), textColor, Rotation,
                           Vector2::Zero, Scale, SpriteEffects::None, 0.0f);
}

inline int MenuEntry::GetHeight(MenuScreen& screen) {
    return screen.getScreenManagerRef().getFont().getLineSpacingProperty();
}

inline int MenuEntry::GetWidth(MenuScreen& screen) {
    return (int)screen.getScreenManagerRef().getFont().MeasureString(text_).X;
}

} // namespace Yacht
