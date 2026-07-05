#pragma once

// StraightLineScalingComponent.hpp — C++ port of
// Elements/General/StraightLineScalingComponent.cs (XNA 4.0 NinjAcademy
// sample). A component that moves in a straight line while scaling.

#include <cmath>

#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

#include "StraightLineMovementComponent.hpp"

namespace NinjAcademy {

// Port of Elements/General/StraightLineScalingComponent.cs.
class StraightLineScalingComponent : public StraightLineMovementComponent {
public:
    float Scale = 1.0f;

    StraightLineScalingComponent(Game& game, GameScreen* gameScreen, Texture2D texture)
        : StraightLineMovementComponent(game, gameScreen, std::move(texture)) {}

    StraightLineScalingComponent(Game& game, GameScreen* gameScreen, Animation animation)
        : StraightLineMovementComponent(game, gameScreen, std::move(animation)) {}

    void Update(GameTime& gameTime) override {
        StraightLineMovementComponent::Update(gameTime);

        float elapsedSeconds = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        float scaleChange = scaleVelocity_ * elapsedSeconds;
        Scale += scaleChange;
        remainingScaleAmount_ -= std::abs(scaleChange);

        // Check whether scaling is complete.
        if (remainingScaleAmount_ <= 0.0f) {
            // The scale may have changed more than intended, so correct it.
            float sign = (scaleChange > 0.0f) ? 1.0f : ((scaleChange < 0.0f) ? -1.0f : 0.0f);
            Scale += remainingScaleAmount_ * -sign;
            scaleVelocity_ = 0.0f;
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        spriteBatch_->Begin();
        animation_.Draw(*spriteBatch_, Position, 0.0f, VisualCenter, Scale, SpriteEffects::None, 0.0f);
        spriteBatch_->End();
    }

    // Moves from initialPosition to destinationPosition while also changing
    // scale, both completing over the same duration.
    void MoveAndScale(System::TimeSpan time, Vector2 initialPosition, Vector2 destinationPosition, float initialScale,
                       float destinationScale) {
        Scale = initialScale;
        float scaleDelta = destinationScale - initialScale;
        remainingScaleAmount_ = std::abs(scaleDelta);
        scaleVelocity_ = scaleDelta / (float)time.getTotalSecondsProperty();

        Move(time, initialPosition, destinationPosition);
    }

private:
    float remainingScaleAmount_ = 0.0f;
    float scaleVelocity_ = 0.0f;
};

} // namespace NinjAcademy
