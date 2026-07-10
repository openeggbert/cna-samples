#pragma once

// MainMenuScreen.hpp — C++ port of Screens/MainMenuScreen.cs (XNA 4.0
// MarbleMaze sample).

#include <memory>

#include "../ScreenManager/MenuScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"
#include "../Misc/AudioManager.hpp"
#include "BackgroundScreen.hpp"

namespace MarbleMazeSample {

class HighScoreScreen; // forward declaration (defined in HighScoreScreen.hpp)
class LoadingAndInstructionScreen; // forward declaration

class MainMenuScreen : public MenuScreen {
public:
    MainMenuScreen() : MenuScreen("") {
        auto startGameMenuEntry = std::make_shared<MenuEntry>("Play");
        auto highScoreMenuEntry = std::make_shared<MenuEntry>("High Score");
        auto exitMenuEntry = std::make_shared<MenuEntry>("Exit");

        startGameMenuEntry->Selected = [this](PlayerIndex) { StartGameMenuEntrySelected(); };
        highScoreMenuEntry->Selected = [this](PlayerIndex) { HighScoreMenuEntrySelected(); };
        exitMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(startGameMenuEntry);
        MenuEntries().push_back(highScoreMenuEntry);
        MenuEntries().push_back(exitMenuEntry);
    }

protected:
    void OnCancel(PlayerIndex playerIndex) override;

private:
    void HighScoreMenuEntrySelected();
    void StartGameMenuEntrySelected();
};

} // namespace MarbleMazeSample
