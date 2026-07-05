#pragma once

// MainMenuScreen.hpp -- C++ port of Screens/MainMenuScreen.cs (XNA 4.0
// CardsStarterKit sample). ThemeGameMenuEntrySelected() is defined
// out-of-line in OptionsMenu.hpp (needs OptionsMenu, which itself needs
// MainMenuScreen::Theme -- a genuine two-way cycle in the original too,
// trivial in C# and resolved here the same way as every other such cycle in
// this port: forward-declare, define once both sides are complete). See
// missing.md.

#include <string>

#include "../../GameStateManagement/MenuScreen.hpp"
#include "../BlackjackCommon.hpp"
#include "GameplayScreen.hpp"

namespace Blackjack {

using GameStateManagement::MenuEntry;
using GameStateManagement::MenuScreen;

class MainMenuScreen : public MenuScreen {
public:
    static std::string Theme;

    MainMenuScreen() : MenuScreen("") {}

    void LoadContent() override {
        auto startGameMenuEntry = std::make_shared<MenuEntry>("Play");
        auto themeGameMenuEntry = std::make_shared<MenuEntry>("Theme");
        auto exitMenuEntry = std::make_shared<MenuEntry>("Exit");

        startGameMenuEntry->Selected = [this](PlayerIndex) { StartGameMenuEntrySelected(); };
        themeGameMenuEntry->Selected = [this](PlayerIndex) { ThemeGameMenuEntrySelected(); };
        exitMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(startGameMenuEntry);
        MenuEntries().push_back(themeGameMenuEntry);
        MenuEntries().push_back(exitMenuEntry);

        MenuScreen::LoadContent();
    }

private:
    void StartGameMenuEntrySelected() {
        for (auto& screen : GetScreenManager()->GetScreens())
            screen->ExitScreen();
        GetScreenManager()->AddScreen(std::make_shared<GameplayScreen>(Theme), std::nullopt);
    }

    void ThemeGameMenuEntrySelected();  // defined in OptionsMenu.hpp

protected:
    void OnCancel(PlayerIndex playerIndex) override {
        (void)playerIndex;
        GetScreenManager()->getGameProperty().Exit();
    }
};

inline std::string MainMenuScreen::Theme = "Red";

} // namespace Blackjack
