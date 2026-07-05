#pragma once

// BackgroundScreen.hpp -- C++ port of Screens/BackgroundScreen.cs (XNA 4.0
// CardsStarterKit sample). Draws the title-screen background image behind
// menu screens.

#include "../../GameStateManagement/GameScreen.hpp"
#include "../../GameStateManagement/ScreenManager.hpp"
#include "../BlackjackCommon.hpp"

namespace Blackjack {

using GameStateManagement::GameScreen;
using GameStateManagement::mul;
using GameStateManagement::ScreenManager;

class BackgroundScreen : public GameScreen {
public:
    BackgroundScreen() {
        setTransitionOnTime(TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        background_ = Load<Texture2D>("Images/titlescreen");
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        (void)coveredByOtherScreen;
        GameScreen::Update(gameTime, otherScreenHasFocus, false);
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        ScreenManager* sm = GetScreenManager();
        sm->getSpriteBatch().Begin();
        sm->getSpriteBatch().Draw(background_, sm->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty(),
                                  mul(Color::White, TransitionAlpha()));
        sm->getSpriteBatch().End();
    }

private:
    Texture2D background_;
};

} // namespace Blackjack
