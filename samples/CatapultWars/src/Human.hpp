#pragma once

#include <optional>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

#include "Player.hpp"
#include "InputState.hpp"

namespace CatapultWars {

using Microsoft::Xna::Framework::Graphics::Texture2D;

// The human player. Aims and fires by dragging the mouse (the XNA original used
// a Windows Phone touch drag gesture — see missing.md).
class Human : public Player {
public:
    Human(Game& game, SpriteBatch& screenSpriteBatch)
        : Player(game, screenSpriteBatch) {
        setCatapult(std::make_shared<Catapult>(
            game, screenSpriteBatch,
            "Textures/Catapults/Blue/blueIdle/blueIdle",
            catapultPosition_, SpriteEffects::None, false));
    }

    bool getIsDragging() const { return isDragging_; }
    void setIsDragging(bool v) { isDragging_ = v; }

    void Initialize() override {
        // File is Arrow.png (the case-insensitive XNA original loaded "arrow").
        arrow_.emplace(game_->getContentProperty().Load<Texture2D>("Textures/HUD/Arrow"));
        catapult_->Initialize();
        Player::Initialize();
    }

    // Processes mouse drag input. Called by GameplayScreen during the human turn.
    void HandleInput(InputState& input) {
        if (!getIsActive())
            return;

        if (input.IsLeftMouseDown()) {
            Vector2 pos = input.MousePosition();

            if (!firstSample_.has_value()) {
                firstSample_ = pos;
                catapult_->setCurrentState(CatapultState::Aiming);
            }

            prevSample_ = pos;

            Vector2 delta = prevSample_.value() - firstSample_.value();
            catapult_->setShotStrength(delta.Length() / maxDragDelta_);
            float baseScale = 0.001f;
            arrowScale_ = baseScale * delta.Length();
            isDragging_ = true;
        } else if (input.IsNewLeftMouseRelease()) {
            if (firstSample_.has_value()) {
                catapult_->setShotVelocity(
                    MinShotStrength + catapult_->getShotStrength() *
                    (MaxShotStrength - MinShotStrength));
                catapult_->Fire(catapult_->getShotVelocity());
                catapult_->setCurrentState(CatapultState::Firing);
            }
            ResetDragState();
        }
    }

    void Draw(const GameTime& gameTime) override {
        if (isDragging_)
            DrawDragArrow(arrowScale_);
        Player::Draw(gameTime);
    }

    void DrawDragArrow(float arrowScale) {
        spriteBatch_->Draw(*arrow_, catapultPosition_ + Vector2(0, -40),
                          std::nullopt, Color::Blue, 0.0f, Vector2::Zero,
                          Vector2(arrowScale, 0.1f), SpriteEffects::None, 0.0f);
    }

    // Turn off dragging state and reset drag-related variables.
    void ResetDragState() {
        firstSample_.reset();
        prevSample_.reset();
        isDragging_ = false;
        arrowScale_ = 0;
        catapult_->setShotStrength(0);
    }

private:
    std::optional<Vector2> prevSample_;
    std::optional<Vector2> firstSample_;
    bool isDragging_ = false;
    // Longest distance possible between drag points (matches the XNA constant).
    const float maxDragDelta_ = Vector2(480, 800).Length();
    std::optional<Texture2D> arrow_;
    float arrowScale_ = 0.0f;

    Vector2 catapultPosition_ = Vector2(140, 332);
};

} // namespace CatapultWars
