#pragma once

// MainMenuScreen.hpp — C++ port of Screens/MainMenuScreen.cs (XNA 4.0
// HoneycombRush sample).

#include "../Misc/AudioManager.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "BackgroundScreen.hpp"
#include "LoadingAndInstructionScreen.hpp"

namespace HoneycombRush {

class HighScoreScreen; // forward declaration — OnCancel() is defined
                        // out-of-line in HighScoreScreen.hpp once the full
                        // type (and its static SaveHighscore()) is known.

class MainMenuScreen : public MenuScreen {
public:
    MainMenuScreen() : MenuScreen("") {
        auto startGameMenuEntry = std::make_shared<MenuEntry>("Start");
        startGameMenuEntry->setPosition(Vector2(173, 364));
        auto exitMenuEntry = std::make_shared<MenuEntry>("Exit");
        exitMenuEntry->setPosition(Vector2(425, 364));

        startGameMenuEntry->Selected = [this](PlayerIndex playerIndex) { StartGameMenuEntrySelected(playerIndex); };
        exitMenuEntry->Selected = [this](PlayerIndex playerIndex) { OnCancel(playerIndex); };

        MenuEntries().push_back(startGameMenuEntry);
        MenuEntries().push_back(exitMenuEntry);
    }

    void LoadContent() override {
        AudioManager::LoadSounds();
        AudioManager::LoadMusic();

        AudioManager::PlayMusic("MenuMusic_Loop");

        MenuScreen::LoadContent();
    }

protected:
    // Defined out-of-line in HighScoreScreen.hpp — needs
    // HighScoreScreen::SaveHighscore(), which is only forward-declared here.
    void OnCancel(PlayerIndex playerIndex) override;

private:
    void StartGameMenuEntrySelected(PlayerIndex) {
        for (auto& screen : GetScreenManager()->GetScreens())
            screen->ExitScreen();

        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("Instructions"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<LoadingAndInstructionScreen>(), std::nullopt);

        AudioManager::StopSound("MenuMusic_Loop");
    }
};

} // namespace HoneycombRush
