#pragma once

// LoadingScreen.hpp -- C++ port of MenuScreens/LoadingScreen.cs. Simplified:
// draws "Loading..." text instead of the original's dedicated loading-image +
// full-screen fade textures (see missing.md).

#include <memory>
#include <vector>

#include "../Fonts.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace RolePlaying {

class LoadingScreen : public GameScreen {
public:
    // Activates the loading screen: tells all current screens to transition
    // off, then adds a LoadingScreen that will add screensToLoad once they're gone.
    static void Load(ScreenManager& screenManager, bool loadingIsSlow,
                      std::vector<std::shared_ptr<GameScreen>> screensToLoad) {
        for (auto& screen : screenManager.GetScreens()) screen->ExitScreen();
        auto loadingScreen = std::shared_ptr<LoadingScreen>(new LoadingScreen(loadingIsSlow, std::move(screensToLoad)));
        screenManager.AddScreen(loadingScreen);
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

        if (otherScreensAreGone_) {
            auto* sm = GetScreenManager();
            sm->RemoveScreen(this);
            for (auto& screen : screensToLoad_)
                if (screen) sm->AddScreen(screen);
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        if (GetScreenState() == ScreenState::Active && GetScreenManager()->GetScreens().size() == 1) {
            otherScreensAreGone_ = true;
        }

        if (loadingIsSlow_) {
            auto& spriteBatch = GetScreenManager()->getSpriteBatch();
            auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
            spriteBatch.Begin();
            Fonts::DrawCenteredText(
                spriteBatch, Fonts::HeaderFont(), "Loading...",
                Microsoft::Xna::Framework::Vector2((float)viewport.getWidthProperty() / 2.0f, (float)viewport.getHeightProperty() / 2.0f),
                Microsoft::Xna::Framework::Color(255, 255, 255, TransitionAlpha()));
            spriteBatch.End();
        }
    }

private:
    LoadingScreen(bool loadingIsSlow, std::vector<std::shared_ptr<GameScreen>> screensToLoad)
        : loadingIsSlow_(loadingIsSlow), screensToLoad_(std::move(screensToLoad)) {
        SetTransitionOnTime(System::TimeSpan::FromSeconds(0.5));
    }

    bool loadingIsSlow_;
    bool otherScreensAreGone_ = false;
    std::vector<std::shared_ptr<GameScreen>> screensToLoad_;
};

} // namespace RolePlaying
