#pragma once

// ScaleGameComponentAnimation.hpp -- C++ port of UI/ScaleGameComponentAnimation.cs
// (XNA 4.0 CardsStarterKit sample). Scales a component over time.

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "AnimatedGameComponent.hpp"
#include "AnimatedGameComponentAnimation.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class ScaleGameComponentAnimation : public AnimatedGameComponentAnimation {
public:
    ScaleGameComponentAnimation(float beginFactor, float endFactor)
        : beginFactor_(beginFactor), factorDelta_(endFactor - beginFactor) {}

    void Run(GameTime& gameTime) override {
        if (IsStarted()) {
            Texture2D* texture = Component->CurrentFrame;
            if (texture != nullptr) {
                percent_ += (float)(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty() /
                                     Duration.getTotalSecondsProperty());

                Rectangle bounds = texture->getBoundsProperty();
                bounds.X = (int)Component->CurrentPosition.X;
                bounds.Y = (int)Component->CurrentPosition.Y;
                float currentFactor = beginFactor_ + factorDelta_ * percent_ - 1;
                bounds.Inflate((int)(bounds.Width * currentFactor), (int)(bounds.Height * currentFactor));
                Component->CurrentDestination = bounds;
            }
        }
    }

private:
    float percent_ = 0;
    float beginFactor_;
    float factorDelta_;
};

} // namespace CardsFramework
