#pragma once

// InstructionScreen.hpp -- C++ port of Screens/InstructionScreen.cs (XNA 4.0
// CardsStarterKit sample). Not reachable from any menu in the original
// sample either (nothing constructs it) -- ported faithfully for
// completeness, not wired up here either. See missing.md.

#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"

#include "../BlackjackCommon.hpp"
#include "GameplayScreen.hpp"

namespace Blackjack {

using GameStateManagement::mul;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::Buttons;

class InstructionScreen : public GameplayScreen {
public:
    explicit InstructionScreen(const std::string& theme) : GameplayScreen(""), theme_(theme) {
        setTransitionOnTime(TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        background_ = Load<Texture2D>("Images/instructions");
        font_ = Load<SpriteFont>("Fonts/MenuFont");
        gameplayScreen_ = std::make_shared<GameplayScreen>(theme_);
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        if (isExit_ && !isExited_) {
            for (auto& screen : GetScreenManager()->GetScreens())
                screen->ExitScreen();

            gameplayScreen_->setScreenManager(GetScreenManager());
            GetScreenManager()->AddScreen(gameplayScreen_, std::nullopt);
            isExited_ = true;
        }

        HandleMouseAndPad(Mouse::GetState(), GamePad::GetState(PlayerIndex::One));

        GameplayScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        auto& spriteBatch = GetScreenManager()->getSpriteBatch();
        spriteBatch.Begin();

        spriteBatch.Draw(background_, GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty(),
                         mul(Color::White, TransitionAlpha()));

        if (isExit_) {
            Rectangle safeArea = GetScreenManager()->SafeArea();
            std::string text = "Loading...";
            Vector2 measure = font_->MeasureString(text);
            Vector2 textPosition((float)(safeArea.X + safeArea.Width / 2) - measure.X / 2.0f,
                                 (float)(safeArea.Y + safeArea.Height / 2) - measure.Y / 2.0f);
            spriteBatch.DrawString(*font_, text, textPosition, Color::Black);
        }

        spriteBatch.End();
    }

private:
    void HandleMouseAndPad(MouseState mouseState, GamePadState padState) {
        if (isExit_) return;

        PlayerIndex result;
        if (mouseState.getLeftButtonProperty() == ButtonState::Pressed) {
            isExit_ = true;
        } else if (GetScreenManager()->input.IsNewButtonPress(Buttons::A, std::nullopt, result) ||
                   GetScreenManager()->input.IsNewButtonPress(Buttons::Start, std::nullopt, result)) {
            isExit_ = true;
        }
        (void)padState;
    }

    Texture2D background_;
    std::optional<SpriteFont> font_;
    std::shared_ptr<GameplayScreen> gameplayScreen_;
    std::string theme_;
    bool isExit_ = false;
    bool isExited_ = false;
};

} // namespace Blackjack
