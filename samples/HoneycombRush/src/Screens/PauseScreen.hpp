#pragma once

// PauseScreen.hpp — C++ port of Screens/PauseScreen.cs (XNA 4.0
// HoneycombRush sample).

#include "../Misc/AudioManager.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "BackgroundScreen.hpp"
#include "MainMenuScreen.hpp"

namespace HoneycombRush {

class GameplayScreen; // forward declaration — ReturnGameMenuEntrySelected()
                       // is defined out-of-line in GameplayScreen.hpp, since
                       // it needs the full type to avoid exiting it.

class PauseScreen : public MenuScreen {
public:
    PauseScreen() : MenuScreen("") {
        setIsPopup(true);

        auto returnGameMenuEntry = std::make_shared<MenuEntry>("Resume");
        returnGameMenuEntry->setPosition(Vector2(173, 364));
        returnGameMenuEntry->Scale = 0.7f;

        auto exitMenuEntry = std::make_shared<MenuEntry>("Exit");
        exitMenuEntry->setPosition(Vector2(425, 364));

        returnGameMenuEntry->Selected = [this](PlayerIndex playerIndex) { ReturnGameMenuEntrySelected(playerIndex); };
        exitMenuEntry->Selected = [this](PlayerIndex playerIndex) { OnCancel(playerIndex); };

        MenuEntries().push_back(returnGameMenuEntry);
        MenuEntries().push_back(exitMenuEntry);
    }

protected:
    void OnCancel(PlayerIndex playerIndex) override {
        (void)playerIndex;

        for (auto& screen : GetScreenManager()->GetScreens())
            screen->ExitScreen();

        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("titleScreen"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
    }

private:
    // Defined out-of-line in GameplayScreen.hpp.
    void ReturnGameMenuEntrySelected(PlayerIndex playerIndex);
};

} // namespace HoneycombRush
