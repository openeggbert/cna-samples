#pragma once

// PauseScreen.hpp -- C++ port of Screens/PauseScreen.cs (XNA 4.0
// CardsStarterKit sample). Also defines, out-of-line, GameplayScreen's
// PauseCurrentGame() and BlackjackCardGame's BackButton_Click() -- both were
// declared earlier but deferred here since they need PauseScreen/
// BackgroundScreen/MainMenuScreen complete (see the forward-declaration
// comments in GameplayScreen.hpp/BlackjackCardGame.hpp and missing.md).

#include <algorithm>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/IGameComponent.hpp"

#include "../../GameStateManagement/MenuScreen.hpp"
#include "../BetGameComponent.hpp"
#include "../BlackjackCommon.hpp"
#include "../InputHelper.hpp"
#include "BackgroundScreen.hpp"
#include "GameplayScreen.hpp"
#include "MainMenuScreen.hpp"

namespace Blackjack {

using CardsFramework::AnimatedGameComponent;
using CardsFramework::GameTable;
using GameStateManagement::GameScreen;
using GameStateManagement::MenuEntry;
using GameStateManagement::MenuScreen;
using GameStateManagement::ScreenManager;
using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::IGameComponent;

class PauseScreen : public MenuScreen {
public:
    PauseScreen() : MenuScreen("") {}

    void LoadContent() override {
        auto returnGameMenuEntry = std::make_shared<MenuEntry>("Back");
        auto exitMenuEntry = std::make_shared<MenuEntry>("Quit");

        returnGameMenuEntry->Selected = [this](PlayerIndex) { ReturnGameMenuEntrySelected(); };
        exitMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(returnGameMenuEntry);
        MenuEntries().push_back(exitMenuEntry);

        MenuScreen::LoadContent();
    }

private:
    void ReturnGameMenuEntrySelected() {
        auto screens = GetScreenManager()->GetScreens();
        std::shared_ptr<GameplayScreen> gameplayScreen;
        std::vector<std::shared_ptr<GameScreen>> rest;

        for (auto& screen : screens) {
            if (auto gp = std::dynamic_pointer_cast<GameplayScreen>(screen))
                gameplayScreen = gp;
            else
                rest.push_back(screen);
        }

        for (auto& screen : rest)
            screen->ExitScreen();

        if (gameplayScreen)
            gameplayScreen->ReturnFromPause();
    }

    void OnCancel(PlayerIndex playerIndex) override {
        (void)playerIndex;

        std::vector<IGameComponent*> toRemove;
        for (auto* component : GetScreenManager()->getGameProperty().getComponentsProperty()) {
            if (!dynamic_cast<ScreenManager*>(component))
                toRemove.push_back(component);
        }
        for (auto* component : toRemove)
            (void)GetScreenManager()->getGameProperty().getComponentsProperty().Remove(component);

        for (auto& screen : GetScreenManager()->GetScreens())
            screen->ExitScreen();

        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
    }
};

// ---- Cross-referencing definitions (need PauseScreen/BackgroundScreen/MainMenuScreen complete) ----

inline void GameplayScreen::PauseCurrentGame() {
    GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
    GetScreenManager()->AddScreen(std::make_shared<PauseScreen>(), std::nullopt);

    pauseEnabledComponents_.clear();
    pauseVisibleComponents_.clear();
    for (auto* component : GetScreenManager()->getGameProperty().getComponentsProperty()) {
        if (dynamic_cast<BetGameComponent*>(component) || dynamic_cast<AnimatedGameComponent*>(component) ||
            dynamic_cast<GameTable*>(component) || dynamic_cast<InputHelper*>(component)) {
            auto* pauseComponent = dynamic_cast<DrawableGameComponent*>(component);
            if (pauseComponent == nullptr) continue;

            if (pauseComponent->getEnabledProperty()) {
                pauseEnabledComponents_.push_back(pauseComponent);
                pauseComponent->setEnabledProperty(false);
            }
            if (pauseComponent->getVisibleProperty()) {
                pauseVisibleComponents_.push_back(pauseComponent);
                pauseComponent->setVisibleProperty(false);
            }
        }
    }
}

inline void BlackjackCardGame::BackButton_Click() {
    std::vector<IGameComponent*> toRemove;
    for (auto* c : GameInstance->getComponentsProperty()) {
        if (!dynamic_cast<ScreenManager*>(c))
            toRemove.push_back(c);
    }
    for (auto* c : toRemove)
        RemoveComponentByRaw(c);

    for (auto& screen : screenManager_->GetScreens())
        screen->ExitScreen();

    screenManager_->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
    screenManager_->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
}

} // namespace Blackjack
