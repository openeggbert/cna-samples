#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "ScreenManager.hpp"
#include "MenuEntry.hpp"

namespace GameStateManagement {

// Base class for screens that contain a menu of options. The user can move up
// and down to select an entry, or cancel to back out of the screen.
class MenuScreen : public GameScreen {
public:
    explicit MenuScreen(const std::string& menuTitle) : menuTitle_(menuTitle) {
        setTransitionOnTime(TimeSpan::FromSeconds(0.5));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void HandleInput(InputState& input) override {
        if (input.IsMenuUp(ControllingPlayer())) {
            selectedEntry_--;
            if (selectedEntry_ < 0)
                selectedEntry_ = (int)menuEntries_.size() - 1;
        }
        if (input.IsMenuDown(ControllingPlayer())) {
            selectedEntry_++;
            if (selectedEntry_ >= (int)menuEntries_.size())
                selectedEntry_ = 0;
        }

        PlayerIndex playerIndex;
        if (input.IsMenuSelect(ControllingPlayer(), playerIndex))
            OnSelectEntry(selectedEntry_, playerIndex);
        else if (input.IsMenuCancel(ControllingPlayer(), playerIndex))
            OnCancel(playerIndex);
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
        Vector2 ms = font.MeasureString(menuTitle_);
        Vector2 titleOrigin(ms.X * 0.5f, ms.Y * 0.5f);
        Color titleColor = mul(Color(192, 192, 192), TransitionAlpha());
        float titleScale = 1.25f;

        titlePosition.Y -= transitionOffset * 100;

        spriteBatch.DrawString(font, menuTitle_, titlePosition, titleColor, 0.0f,
                               titleOrigin, titleScale, SpriteEffects::None, 0.0f);

        spriteBatch.End();
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

    // Positions the menu entries. By default all entries are lined up in a
    // vertical list, centered on the screen.
    virtual void UpdateMenuEntryLocations() {
        float transitionOffset = (float)std::pow(TransitionPosition(), 2);

        Vector2 position(0.0f, 175.0f);

        for (size_t i = 0; i < menuEntries_.size(); i++) {
            auto& menuEntry = menuEntries_[i];

            position.X = (float)(screenManager_->getGraphicsDeviceProperty()
                                     .getViewportProperty().getWidthProperty() / 2)
                         - menuEntry->GetWidth(*this) / 2.0f;

            if (GetScreenState() == ScreenState::TransitionOn)
                position.X -= transitionOffset * 256;
            else
                position.X += transitionOffset * 512;

            menuEntry->setPosition(position);
            position.Y += menuEntry->GetHeight(*this);
        }
    }

private:
    std::vector<std::shared_ptr<MenuEntry>> menuEntries_;
    int selectedEntry_ = 0;
    std::string menuTitle_;
};

// ---- MenuEntry methods that depend on MenuScreen / ScreenManager ----

inline void MenuEntry::Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime) {
    Color color = isSelected ? Color::Yellow : Color::White;

    double time = gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();
    float pulsate = (float)std::sin(time * 6) + 1;
    float scale = 1 + pulsate * 0.05f * selectionFade_;

    color = mul(color, screen.TransitionAlpha());

    ScreenManager* sm = screen.GetScreenManager();
    SpriteBatch& spriteBatch = sm->getSpriteBatch();
    SpriteFont& font = sm->getFont();

    Vector2 origin(0.0f, font.getLineSpacingProperty() / 2.0f);

    spriteBatch.DrawString(font, text_, position_, color, 0.0f, origin, scale,
                           SpriteEffects::None, 0.0f);
}

inline int MenuEntry::GetHeight(MenuScreen& screen) {
    return screen.GetScreenManager()->getFont().getLineSpacingProperty();
}

inline int MenuEntry::GetWidth(MenuScreen& screen) {
    return (int)screen.GetScreenManager()->getFont().MeasureString(text_).X;
}

} // namespace GameStateManagement
