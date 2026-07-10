#pragma once

// ScreensGlue.hpp (NOXNA organizational file, no C# equivalent) — the 6 screen
// classes in this sample's ScreenManager stack reference each other circularly
// (MainMenuScreen <-> HighScoreScreen, PauseScreen <-> MainMenuScreen,
// PauseScreen <-> GameplayScreen, GameplayScreen <-> HighScoreScreen,
// LoadingAndInstructionScreen -> GameplayScreen), exactly as XNA's own C# original
// does -- unremarkable there since the whole project compiles as one unit, but
// C++ needs the mutual references broken via forward declarations (each screen's
// own header only forward-declares the other screens it needs, per that header's
// own comments) plus deferred method bodies. This file includes every screen
// header (order doesn't matter now -- each is already self-sufficient given its
// forward declarations) and defines every deferred method body, once all 6
// classes are fully visible to each other. Same technique this repo's
// GameScreen.hpp/ScreenManager.hpp/MenuEntry.hpp/MenuScreen.hpp already use for
// their own (smaller) mutual reference.

#include <memory>
#include <optional>

#include "BackgroundScreen.hpp"
#include "GameplayScreen.hpp"
#include "HighScoreScreen.hpp"
#include "LoadingAndInstructionScreen.hpp"
#include "MainMenuScreen.hpp"
#include "PauseScreen.hpp"

namespace MarbleMazeSample {

// ---- HighScoreScreen ----

inline void HighScoreScreen::HandleInput(InputState& input) {
    if (input.IsPauseGame(std::nullopt)) {
        Exit();
    }

    if (!input.Gestures.empty()) {
        if (input.Gestures[0].getGestureTypeProperty() == GestureType::Tap) {
            Exit();
            input.Gestures.clear();
        }
    }
}

inline void HighScoreScreen::Exit() {
    ExitScreen();
    GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
    GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
}

// ---- PauseScreen ----

inline void PauseScreen::ReturnGameMenuEntrySelected() {
    AudioManager::PauseResumeSounds(true);

    for (auto& screen : GetScreenManager()->GetScreens()) {
        if (dynamic_cast<GameplayScreen*>(screen.get()) == nullptr) screen->ExitScreen();
    }

    auto gameplay = std::dynamic_pointer_cast<GameplayScreen>(GetScreenManager()->GetScreens()[0]);
    gameplay->IsActive = true;
}

inline void PauseScreen::RestartGameMenuEntrySelected() {
    AudioManager::PauseResumeSounds(true);

    for (auto& screen : GetScreenManager()->GetScreens()) {
        if (dynamic_cast<GameplayScreen*>(screen.get()) == nullptr) screen->ExitScreen();
    }

    auto gameplay = std::dynamic_pointer_cast<GameplayScreen>(GetScreenManager()->GetScreens()[0]);
    gameplay->IsActive = true;
    gameplay->Restart();
}

inline void PauseScreen::OnCancel(PlayerIndex playerIndex) {
    (void)playerIndex;
    for (auto& screen : GetScreenManager()->GetScreens()) screen->ExitScreen();

    GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
    GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
}

// ---- GameplayScreen ----

inline void GameplayScreen::FinishCurrentGame() {
    IsActive = false;

    for (auto& screen : GetScreenManager()->GetScreens()) screen->ExitScreen();

    if (HighScoreScreen::IsInHighscores(elapsedGameTime_)) {
        // Simplification (see missing.md): always names the entry "Player"
        // instead of the original's Guide.BeginShowKeyboardInput on-screen
        // keyboard dialog.
        HighScoreScreen::PutHighScore("Player", elapsedGameTime_);
    }

    GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
    GetScreenManager()->AddScreen(std::make_shared<HighScoreScreen>(), std::nullopt);
}

inline void GameplayScreen::PauseCurrentGame() {
    IsActive = false;
    AudioManager::PauseResumeSounds(false);

    GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
    GetScreenManager()->AddScreen(std::make_shared<PauseScreen>(), std::nullopt);
}

inline bool GameplayScreen::HighScoreIsInHighscores() const { return HighScoreScreen::IsInHighscores(elapsedGameTime_); }

// ---- LoadingAndInstructionScreen ----

inline void LoadingAndInstructionScreen::LoadContent() {
    background_ = Load<Texture2D>("Textures/instructions");
    font_ = Load<SpriteFont>("Fonts/MenuFont");

    gameplayScreen_ = std::make_shared<GameplayScreen>();
    gameplayScreen_->setScreenManager(GetScreenManager());
}

inline void LoadingAndInstructionScreen::HandleInput(InputState& input) {
    if (!isLoading_) {
        if (!input.Gestures.empty() && input.Gestures[0].getGestureTypeProperty() == GestureType::Tap) {
            isLoading_ = true;
        }
    }
}

inline void LoadingAndInstructionScreen::Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) {
    if (isLoading_) {
        if (!loadedThisFrame_) {
            // Synchronous load (see file header comment) -- the C# original
            // does this on a background thread instead.
            loadedThisFrame_ = true;
            gameplayScreen_->LoadAssets();
        } else if (!IsExiting()) {
            for (auto& screen : GetScreenManager()->GetScreens()) screen->ExitScreen();
            GetScreenManager()->AddScreen(gameplayScreen_, std::nullopt);
        }
    }

    GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
}

// ---- MainMenuScreen ----

inline void MainMenuScreen::HighScoreMenuEntrySelected() {
    for (auto& screen : GetScreenManager()->GetScreens()) screen->ExitScreen();

    GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
    GetScreenManager()->AddScreen(std::make_shared<HighScoreScreen>(), std::nullopt);
}

inline void MainMenuScreen::StartGameMenuEntrySelected() {
    for (auto& screen : GetScreenManager()->GetScreens()) screen->ExitScreen();

    GetScreenManager()->AddScreen(std::make_shared<LoadingAndInstructionScreen>(), std::nullopt);
}

inline void MainMenuScreen::OnCancel(PlayerIndex playerIndex) {
    (void)playerIndex;
    HighScoreScreen::SaveHighscore();
    GetScreenManager()->getGameProperty().Exit();
}

} // namespace MarbleMazeSample
