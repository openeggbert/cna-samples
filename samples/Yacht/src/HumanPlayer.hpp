#pragma once

#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"

#include "YachtPlayer.hpp"
#include "DiceHandler.hpp"
#include "Button.hpp"
#include "InputState.hpp"
#include "Accelerometer.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;

// The human player. Ported from Objects/HumanPlayer.cs; the GameTypes/
// NetworkManager branches (online-only ResetTimeout calls) are dropped per
// the approved plan.
//
// A handful of methods that need GameStateHandler's full type are only
// *declared* here and *defined* out-of-line at the bottom of
// GameStateHandler.hpp -- mirroring the Catapult<->Player cycle-break idiom
// already used by CatapultWars (see NEXT.md): GameStateHandler owns and
// constructs HumanPlayer, while HumanPlayer calls back into it.
class HumanPlayer : public YachtPlayer {
public:
    HumanPlayer(const std::string& name, DiceHandler* diceHandler, InputState& input,
               Rectangle screenBounds, SpriteFont& font)
        : YachtPlayer(name, diceHandler), input_(&input), screenBounds_(screenBounds), font_(&font) {}

    void LoadAssets(ContentManager& contentManager) {
        rollTexture_.emplace(contentManager.Load<Texture2D>("Images/rollBtn"));
        scoreTexture_.emplace(contentManager.Load<Texture2D>("Images/scoreBtn"));

        Vector2 position(
            (float)(screenBounds_.getRightProperty() - rollTexture_->getWidthProperty() - 10),
            (float)(screenBounds_.getCenterProperty().Y - rollTexture_->getBoundsProperty().Height));
        roll_.emplace(*rollTexture_, position, nullptr, "");

        position.X -= (float)(scoreTexture_->getWidthProperty() + 20);
        score_.emplace(*scoreTexture_, position, nullptr, "");

        roll_->Click = [this]() { HandleRollButtonClick(); };
        score_->Click = [this]() { HandleScoreButtonClick(); };
    }

    void Draw(SpriteBatch& spriteBatch) override {
        roll_->Draw(spriteBatch);
        score_->Draw(spriteBatch);

        DrawRollCounter(spriteBatch);
        DrawSelectedScore(spriteBatch);
    }

    void PerformPlayerLogic() override {
        roll_->Enabled = diceHandler_->getRolls() != 3 && !diceHandler_->DiceRolling();
        score_->Enabled = CanSelectScore();

        for (const auto& gesture : input_->Gestures) {
            roll_->HandleInput(gesture);
            score_->HandleInput(gesture);
            HandleDiceHandlerInput(gesture);
            HandleSelectScoreInput(gesture);
        }

        // Mouse: desktop parallel to the gesture loop above (see missing.md).
        roll_->HandleInput(*input_);
        score_->HandleInput(*input_);
        HandleDiceHandlerMouseInput(*input_);
        HandleSelectScoreMouseInput(*input_);

        HandleShakeInput();
    }

private:
    void DrawRollCounter(SpriteBatch& spriteBatch) {
        if (diceHandler_->getRolls() < 3) {
            std::string text = "ROLLS";
            Vector2 measure = font_->MeasureString(text);
            Vector2 position((float)roll_->Position.X, (float)roll_->Position.Y);
            position.Y += (float)(roll_->getTexture().getHeightProperty() + 10);
            position.X += (float)roll_->getTexture().getBoundsProperty().getCenterProperty().X - measure.X / 2.0f;
            spriteBatch.DrawString(*font_, text, position, Color::White);

            text = "X" + std::to_string(3 - diceHandler_->getRolls());
            position.Y += measure.Y;
            measure = font_->MeasureString(text);
            position.X = (float)roll_->Position.X;
            position.X += (float)roll_->getTexture().getBoundsProperty().getCenterProperty().X - measure.X / 2.0f;
            spriteBatch.DrawString(*font_, text, position, Color::White);
        }
    }

    // ---- Deferred: defined in GameStateHandler.hpp (need its full type) ----
    bool CanSelectScore() const;
    void DrawSelectedScore(SpriteBatch& spriteBatch);
    void TryMoveDiceAt(Vector2 point);
    void TrySelectScoreAt(Vector2 point);
    void HandleScoreButtonClick();

    void HandleDiceHandlerInput(const GestureSample& sample) {
        if (diceHandler_->getRolls() < 3 && sample.getGestureTypeProperty() == GestureType::Tap)
            TryMoveDiceAt(sample.getPositionProperty());
    }

    void HandleDiceHandlerMouseInput(InputState& input) {
        if (diceHandler_->getRolls() < 3 && input.IsNewLeftMousePress())
            TryMoveDiceAt(input.MousePosition());
    }

    void HandleSelectScoreInput(const GestureSample& sample) {
        if (sample.getGestureTypeProperty() == GestureType::Tap)
            TrySelectScoreAt(sample.getPositionProperty());
    }

    void HandleSelectScoreMouseInput(InputState& input) {
        if (input.IsNewLeftMousePress())
            TrySelectScoreAt(input.MousePosition());
    }

    void HandleRollButtonClick() {
        diceHandler_->Roll();
    }

    void HandleShakeInput() {
        if (!registeredForShakeDetection_) {
            Accelerometer::ShakeDetected = [this]() { shakeDetect_ = true; };
            registeredForShakeDetection_ = true;
        }

        // Drives the accelerometer (or keyboard-fallback) shake-detection
        // poll; see Accelerometer.hpp for why this must run every frame.
        Accelerometer::GetState();

        if (shakeDetect_) {
            diceHandler_->Roll();
            shakeDetect_ = false;
        }
    }

    InputState* input_;
    Rectangle screenBounds_;
    SpriteFont* font_;

    std::optional<Texture2D> rollTexture_;
    std::optional<Texture2D> scoreTexture_;
    std::optional<Button> roll_;
    std::optional<Button> score_;

    bool registeredForShakeDetection_ = false;
    bool shakeDetect_ = false;
};

} // namespace Yacht
