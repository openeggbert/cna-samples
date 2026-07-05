#pragma once

// MenuScreen.hpp -- C++ port of ScreenManager/MenuScreen.cs. Unlike other
// ScreenManager ports in this repo, navigation here is purely InputManager-
// driven cursor up/down + Ok/Back/ExitGame (no Tap-gesture/mouse hit-testing
// -- this sample never had touch or mouse input in the original).

#include <memory>
#include <vector>

#include "../AudioManager.hpp"
#include "../InputManager.hpp"
#include "GameScreen.hpp"
#include "MenuEntry.hpp"
#include "ScreenManager.hpp"

namespace RolePlaying {

class MenuScreen : public GameScreen {
public:
    MenuScreen() {
        SetTransitionOnTime(System::TimeSpan::FromSeconds(0.5));
        SetTransitionOffTime(System::TimeSpan::FromSeconds(0.5));
    }

    void HandleInput() override {
        int oldSelectedEntry = selectedEntry_;

        if (InputManager::IsActionTriggered(InputManager::Action::CursorUp)) {
            selectedEntry_--;
            if (selectedEntry_ < 0) selectedEntry_ = (int)menuEntries_.size() - 1;
        }
        if (InputManager::IsActionTriggered(InputManager::Action::CursorDown)) {
            selectedEntry_++;
            if (selectedEntry_ >= (int)menuEntries_.size()) selectedEntry_ = 0;
        }

        if (InputManager::IsActionTriggered(InputManager::Action::Ok)) {
            AudioManager::PlayCue("Continue");
            OnSelectEntry(selectedEntry_);
        } else if (InputManager::IsActionTriggered(InputManager::Action::Back) ||
                   InputManager::IsActionTriggered(InputManager::Action::ExitGame)) {
            OnCancel();
        } else if (selectedEntry_ != oldSelectedEntry) {
            AudioManager::PlayCue("MenuMove");
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
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();
        spriteBatch.Begin();
        for (size_t i = 0; i < menuEntries_.size(); i++) {
            bool isSelected = IsActive() && ((int)i == selectedEntry_);
            menuEntries_[i]->Draw(*this, isSelected, gameTime);
        }
        spriteBatch.End();
    }

protected:
    std::vector<std::shared_ptr<MenuEntry>>& MenuEntries() { return menuEntries_; }

    std::shared_ptr<MenuEntry> SelectedMenuEntry() const {
        if (selectedEntry_ < 0 || selectedEntry_ >= (int)menuEntries_.size()) return nullptr;
        return menuEntries_[selectedEntry_];
    }

    virtual void OnSelectEntry(int entryIndex) { menuEntries_[entryIndex]->OnSelectEntry(); }
    virtual void OnCancel() { ExitScreen(); }

    int selectedEntry_ = 0;

private:
    std::vector<std::shared_ptr<MenuEntry>> menuEntries_;
};

// ---- MenuEntry methods that depend on ScreenManager (defined here) ----

inline void MenuEntry::Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime) {
    (void)gameTime;
    Microsoft::Xna::Framework::Color color = isSelected ? Fonts::MenuSelectedColor : Fonts::TitleColor;

    ScreenManager* screenManager = screen.GetScreenManager();
    SpriteBatch& spriteBatch = screenManager->getSpriteBatch();

    if (EntryTexture) {
        spriteBatch.Draw(*EntryTexture, Position, Microsoft::Xna::Framework::Color(255, 255, 255, 255));
        if (Font && !Text().empty()) {
            Vector2 textSize = Font->MeasureString(Text());
            Vector2 textPosition =
                Position + Vector2(std::floor((EntryTexture->getWidthProperty() - textSize.X) / 2.0f),
                                    std::floor((EntryTexture->getHeightProperty() - textSize.Y) / 2.0f));
            spriteBatch.DrawString(*Font, Text(), textPosition, color);
        }
    } else if (Font && !Text().empty()) {
        spriteBatch.DrawString(*Font, Text(), Position, color);
    }
}

} // namespace RolePlaying
