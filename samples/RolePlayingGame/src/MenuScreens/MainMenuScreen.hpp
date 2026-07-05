#pragma once

// MainMenuScreen.hpp -- simplified adaptation of MenuScreens/MainMenuScreen.cs.
// The original draws a full-screen plank/parchment background with a
// game-logo icon and per-entry wooden-plank textures; this port keeps New
// Game / Exit (Save Game / Load Game / Controls / Help are dropped along
// with SaveLoadScreen/ControlsScreen/HelpScreen -- see missing.md) and draws
// plain text entries instead of plank art.

#include <memory>

#include "../AudioManager.hpp"
#include "../Data/ContentLoader.hpp"
#include "../Data/GameStartDescription.hpp"
#include "../Fonts.hpp"
#include "../GameScreens/GameplayScreen.hpp"
#include "../ScreenManager/MenuEntry.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "../Session/Session.hpp"
#include "LoadingScreen.hpp"

namespace RolePlaying {

class MainMenuScreen : public MenuScreen {
public:
    explicit MainMenuScreen(RolePlayingGameData::ContentLoader& contentLoader) : contentLoader_(&contentLoader) {
        auto newGame = std::make_shared<MenuEntry>("New Game");
        newGame->Selected = [this]() { NewGameSelected(); };
        auto exitGame = std::make_shared<MenuEntry>("Exit");
        exitGame->Selected = [this]() { OnCancel(); };

        MenuEntries().push_back(newGame);
        MenuEntries().push_back(exitGame);

        AudioManager::PushMusic("MainTheme");
    }

    void LoadContent() override {
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        float x = (float)(viewport.getWidthProperty() / 2 - 100);
        float y = (float)(viewport.getHeightProperty() / 2 - 40);
        for (auto& entry : MenuEntries()) {
            entry->Font = &Fonts::HeaderFont();
            entry->Position = Microsoft::Xna::Framework::Vector2(x, y);
            y += 60.0f;
        }
        MenuScreen::LoadContent();
    }

protected:
    void OnCancel() override { GetScreenManager()->getGameProperty().Exit(); }

private:
    void NewGameSelected() {
        if (Session::IsActive()) ExitScreen();

        std::vector<std::shared_ptr<GameScreen>> toLoad;
        auto gameStartDescription = contentLoader_->LoadGameStartDescription("MainGameDescription");
        toLoad.push_back(std::make_shared<GameplayScreen>(gameStartDescription, *contentLoader_));
        LoadingScreen::Load(*GetScreenManager(), true, toLoad);
    }

    RolePlayingGameData::ContentLoader* contentLoader_;
};

// ---- GameplayScreen methods that depend on MainMenuScreen (defined here) ----
// The original's CharacterManagement action opens a StatisticsScreen, which
// this port does not implement (see missing.md) -- only MainMenu/ExitGame are
// handled, and ExitGame exits immediately rather than confirming via
// MessageBoxScreen (also not ported).

inline void GameplayScreen::HandleInput() {
    if (InputManager::IsActionTriggered(InputManager::Action::MainMenu)) {
        GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(contentLoader_));
        return;
    }
    if (InputManager::IsActionTriggered(InputManager::Action::ExitGame)) {
        GetScreenManager()->getGameProperty().Exit();
        return;
    }
}

} // namespace RolePlaying
