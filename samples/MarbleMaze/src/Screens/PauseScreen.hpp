#pragma once

// PauseScreen.hpp — C++ port of Screens/PauseScreen.cs (XNA 4.0 MarbleMaze
// sample).

#include <memory>

#include "../ScreenManager/MenuScreen.hpp"
#include "../Misc/AudioManager.hpp"
#include "BackgroundScreen.hpp"

namespace MarbleMazeSample {

class MainMenuScreen; // forward declaration -- OnCancel's body constructs one
                       // (deferred to Screens/ScreensGlue.hpp).
class GameplayScreen;  // forward declaration -- the two Selected handlers below
                        // reach into it (deferred).

class PauseScreen : public MenuScreen {
public:
    PauseScreen() : MenuScreen("Game Paused") {
        auto returnGameMenuEntry = std::make_shared<MenuEntry>("Return");
        auto restartGameMenuEntry = std::make_shared<MenuEntry>("Restart");
        auto exitMenuEntry = std::make_shared<MenuEntry>("Quit Game");

        returnGameMenuEntry->Selected = [this](PlayerIndex) { ReturnGameMenuEntrySelected(); };
        restartGameMenuEntry->Selected = [this](PlayerIndex) { RestartGameMenuEntrySelected(); };
        exitMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(returnGameMenuEntry);
        MenuEntries().push_back(restartGameMenuEntry);
        MenuEntries().push_back(exitMenuEntry);
    }

protected:
    void OnCancel(PlayerIndex playerIndex) override;

private:
    void ReturnGameMenuEntrySelected();
    void RestartGameMenuEntrySelected();
};

} // namespace MarbleMazeSample
