#pragma once

// GameOverScreen.hpp -- simplified adaptation of GameScreens/GameOverScreen.cs.

#include "../AudioManager.hpp"
#include "../Data/ContentLoader.hpp"
#include "../Fonts.hpp"
#include "../InputManager.hpp"
#include "../MenuScreens/MainMenuScreen.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace RolePlaying {

class GameOverScreen : public GameScreen {
public:
    explicit GameOverScreen(RolePlayingGameData::ContentLoader& contentLoader) : contentLoader_(&contentLoader) {
        SetTransitionOnTime(System::TimeSpan::FromSeconds(1.0));
    }

    void HandleInput() override {
        if (InputManager::IsActionTriggered(InputManager::Action::Ok)) {
            GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(*contentLoader_));
            ExitScreen();
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        auto& spriteBatch = GetScreenManager()->getSpriteBatch();
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        spriteBatch.Begin();
        Fonts::DrawCenteredText(
            spriteBatch, Fonts::HeaderFont(), "Game Over",
            Microsoft::Xna::Framework::Vector2((float)viewport.getWidthProperty() / 2.0f, (float)viewport.getHeightProperty() / 2.0f),
            Fonts::TitleColor);
        spriteBatch.End();
    }

private:
    RolePlayingGameData::ContentLoader* contentLoader_;
};

} // namespace RolePlaying
