#pragma once

// PauseScreen.hpp — C++ port of Screens/PauseScreen.cs (XNA 4.0 NinjAcademy
// sample).

#include <memory>

#include "../AudioManager.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "BackgroundScreen.hpp"
#include "MainMenuScreen.hpp"

namespace NinjAcademy {

class GameplayScreen;  // ResumeSelected() is defined out-of-line in
class CountdownScreen; // GameplayScreen.hpp, since it needs both full types.

// Port of Screens/PauseScreen.cs.
class PauseScreen : public MenuScreen {
public:
    // The screen from which the game was paused.
    explicit PauseScreen(GameScreen* screenToRestore) : MenuScreen(""), screenToRestore_(screenToRestore) {
        auto resumeMenuEntry = std::make_shared<MenuEntry>("Resume");
        auto exitMenuEntry = std::make_shared<MenuEntry>("Quit");

        resumeMenuEntry->Selected = [this](PlayerIndex p) { ResumeSelected(p); };
        exitMenuEntry->Selected = [this](PlayerIndex p) { ExitSelected(p); };

        MenuEntries().push_back(resumeMenuEntry);
        MenuEntries().push_back(exitMenuEntry);
    }

    void LoadContent() override {
        for (auto& entry : MenuEntries()) {
            float entryWidth = GetScreenManager()->getFont().MeasureString(entry->Text()).X;
            if (maxEntryWidth_ < entryWidth)
                maxEntryWidth_ = entryWidth;
        }

        MenuScreen::LoadContent();
    }

protected:
    // Arranges the pause screen's menu entries.
    void UpdateMenuEntryLocations() override {
        int menuEntryVerticalPlacement = GameConstants::PauseMenuTop;

        for (auto& entry : MenuEntries()) {
            float entryWidth = GetScreenManager()->getFont().MeasureString(entry->Text()).X;
            int menuEntryHorizontalPlacement =
                (int)(GameConstants::PauseMenuLeft + maxEntryWidth_ / 2 - entryWidth / 2);
            entry->setPosition(Vector2((float)menuEntryHorizontalPlacement, (float)menuEntryVerticalPlacement));
            menuEntryVerticalPlacement += GameConstants::MainMenuEntryGap;
        }
    }

private:
    // Defined out-of-line in GameplayScreen.hpp (needs GameplayScreen/CountdownScreen).
    void ResumeSelected(PlayerIndex playerIndex);

    void ExitSelected(PlayerIndex playerIndex) {
        (void)playerIndex;
        for (auto& screen : GetScreenManager()->GetScreens())
            screen->ExitScreen();

        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("titlescreenBG"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);

        AudioManager::PlayMusic("Menu Music");
    }

    float maxEntryWidth_ = 0.0f;
    GameScreen* screenToRestore_;
};

} // namespace NinjAcademy
