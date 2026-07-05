#pragma once

// GameplayScreen.hpp -- C++ port of GameScreens/GameplayScreen.cs. SaveGame-
// description construction path is dropped (see missing.md); only the
// GameStartDescription path remains. The original's ExitGame action shows a
// MessageBoxScreen confirmation and CharacterManagement opens a
// StatisticsScreen -- both omitted in this port (MessageBoxScreen/
// StatisticsScreen are not ported; see missing.md) -- ExitGame exits
// immediately instead of confirming.

#include <memory>

#include "../Data/GameStartDescription.hpp"
#include "../InputManager.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"
#include "../Session/Session.hpp"
#include "../Combat/CombatEngine.hpp"

namespace RolePlaying {

class GameplayScreen : public GameScreen {
public:
    // contentLoader must outlive this screen -- owned by the top-level Game object.
    GameplayScreen(std::shared_ptr<RolePlayingGameData::GameStartDescription> gameStartDescription,
                   RolePlayingGameData::ContentLoader& contentLoader)
        : gameStartDescription_(std::move(gameStartDescription)), contentLoader_(contentLoader) {
        CombatEngine::ClearCombat();
        // Mirrors the original's Exiting event hookup -- EndSession must be
        // re-entrant safe, since EndSession may itself be closing this screen.
        Exiting = [] { Session::EndSession(); };
    }

    void LoadContent() override {
        Session::StartNewSession(*gameStartDescription_, *GetScreenManager(), *this, contentLoader_);
        GetScreenManager()->getGameProperty().ResetElapsedTime();
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
        if (IsActive() && !coveredByOtherScreen) Session::Update(gameTime);
    }

    // Defined out-of-line -- needs MainMenuScreen (see GameplayScreen.hpp's
    // bottom cross-referencing definitions, assembled in RolePlayingGame.hpp).
    void HandleInput() override;

    void Draw(const GameTime& gameTime) override { Session::Draw(gameTime); }

private:
    std::shared_ptr<RolePlayingGameData::GameStartDescription> gameStartDescription_;
    RolePlayingGameData::ContentLoader& contentLoader_;
};

} // namespace RolePlaying
