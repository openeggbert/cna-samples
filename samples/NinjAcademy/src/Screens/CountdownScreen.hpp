#pragma once

// CountdownScreen.hpp — C++ port of Screens/CountdownScreen.cs (XNA 4.0
// NinjAcademy sample). Displayed before gameplay actually begins, showing a
// 3-2-1 countdown; exits once the countdown finishes.

#include <memory>

#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../ScreenManager/GameScreen.hpp"
#include "BackgroundScreen.hpp"
#include "PauseScreen.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class GameplayScreen; // Update() is defined out-of-line in GameplayScreen.hpp.

// Port of Screens/CountdownScreen.cs.
class CountdownScreen : public GameScreen {
public:
    explicit CountdownScreen(std::shared_ptr<GameplayScreen> gameplayScreen) : gameplayScreen_(std::move(gameplayScreen)) {}

    void LoadContent() override {
        backgroundTexture_.emplace(Load<Texture2D>("Textures/Backgrounds/gameplayBG"));
        roomTexture_.emplace(Load<Texture2D>("Textures/Backgrounds/room"));

        countdownFont_.emplace(Load<SpriteFont>("Fonts/GameScreenFont36px"));

        viewport_ = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty();
        screenCenter_ = Vector2((float)viewport_.getCenterProperty().X, (float)viewport_.getCenterProperty().Y);
    }

    // Defined out-of-line in GameplayScreen.hpp (calls gameplayScreen_->PreDisplayInitialization()).
    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;

    void HandleInput(InputState& input) override {
        GameScreen::HandleInput(input);

        if (input.IsPauseGame(std::nullopt)) {
            PauseCountdown();

            GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("titlescreenBG"), std::nullopt);
            GetScreenManager()->AddScreen(std::make_shared<PauseScreen>(this), std::nullopt);
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        std::string countdownString = std::to_string(countdownValue_);
        Vector2 stringSize = countdownFont_->MeasureString(countdownString);

        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();
        spriteBatch.Draw(*backgroundTexture_, viewport_, Color::White);
        spriteBatch.Draw(*roomTexture_, viewport_, Color::White);
        spriteBatch.DrawString(*countdownFont_, countdownString, screenCenter_ - (stringSize / 2.0f), Color::White);
        spriteBatch.End();
    }

    // Pauses/resumes the countdown.
    void PauseCountdown() { isUpdating_ = false; }
    void ResumeCountdown() { isUpdating_ = true; }

private:
    bool isUpdating_ = true;

    std::optional<Texture2D> backgroundTexture_;
    std::optional<Texture2D> roomTexture_;

    std::optional<SpriteFont> countdownFont_;

    Rectangle viewport_;
    Vector2 screenCenter_;

    System::TimeSpan countdownInterval_ = System::TimeSpan::FromSeconds(1);
    int countdownValue_ = 3;
    System::TimeSpan intervalTimer_ = System::TimeSpan::Zero;

    std::shared_ptr<GameplayScreen> gameplayScreen_;
};

} // namespace NinjAcademy
