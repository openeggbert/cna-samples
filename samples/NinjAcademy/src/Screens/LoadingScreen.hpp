#pragma once

// LoadingScreen.hpp — C++ port of Screens/LoadingScreen.cs (XNA 4.0
// NinjAcademy sample). Background-thread asset loading (System.Threading)
// is simplified to a synchronous LoadAssets() call -- this sample's asset
// set loads fast enough that the original's loading-screen thread has no
// perceptible benefit on desktop; see missing.md.

#include <memory>

#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"

#include "../AudioManager.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "BackgroundScreen.hpp"
#include "MainMenuScreen.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Buttons;

class GameplayScreen;  // defined out-of-line in GameplayScreen.hpp.
class CountdownScreen;

// Port of Screens/LoadingScreen.cs.
class LoadingScreen : public GameScreen {
public:
    LoadingScreen(Texture2D idleTexture, Texture2D busyTexture)
        : idleTexture_(std::move(idleTexture)), busyTexture_(std::move(busyTexture)) {
        setTransitionOnTime(System::TimeSpan::FromSeconds(0));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.5));
        setEnabledGestures(GestureType::Tap);
    }

    // Defined out-of-line in GameplayScreen.hpp (creates a GameplayScreen).
    void LoadContent() override;

    void HandleInput(InputState& input) override {
        if (!isLoading_) {
            PlayerIndex player;

            if (!input.Gestures.empty()) {
                if (input.Gestures[0].getGestureTypeProperty() == GestureType::Tap) {
                    LoadResources();
                }
            } else if (input.IsNewButtonPress(Buttons::Back, ControllingPlayer(), player)) {
                for (auto& screen : GetScreenManager()->GetScreens())
                    screen->ExitScreen();

                GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("titlescreenBG"), std::nullopt);
                GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);

                AudioManager::PlayMusic("Menu Music");
            }
        }

        GameScreen::HandleInput(input);
    }

    // Defined out-of-line in GameplayScreen.hpp (adds a GameplayScreen + CountdownScreen).
    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();
        spriteBatch.Draw(isLoading_ ? busyTexture_ : idleTexture_, viewport_, mul(Color::White, TransitionAlpha()));
        spriteBatch.End();
    }

private:
    void LoadResources();

    Texture2D idleTexture_;
    Texture2D busyTexture_;

    Rectangle viewport_;

    bool isLoading_ = false;
    bool loadFinished_ = false;
    std::shared_ptr<GameplayScreen> gameplayScreen_;
};

} // namespace NinjAcademy
