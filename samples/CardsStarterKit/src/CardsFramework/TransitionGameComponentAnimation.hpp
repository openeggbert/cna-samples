#pragma once

// TransitionGameComponentAnimation.hpp -- C++ port of
// UI/TransitionGameComponentAnimation.cs (XNA 4.0 CardsStarterKit sample).
// Moves a component from one point to another.

#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "AnimatedGameComponent.hpp"
#include "AnimatedGameComponentAnimation.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::Vector2;

class TransitionGameComponentAnimation : public AnimatedGameComponentAnimation {
public:
    TransitionGameComponentAnimation(Vector2 sourcePosition, Vector2 destinationPosition)
        : sourcePosition_(sourcePosition), destinationPosition_(destinationPosition),
          positionDelta_(destinationPosition - sourcePosition) {}

    void Run(GameTime& gameTime) override {
        if (IsStarted()) {
            percent_ += (float)(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty() /
                                 Duration.getTotalSecondsProperty());

            Component->CurrentPosition = sourcePosition_ + positionDelta_ * percent_;

            if (IsDone()) {
                Component->CurrentPosition = destinationPosition_;
            }
        }
    }

private:
    Vector2 sourcePosition_;
    Vector2 positionDelta_;
    float percent_ = 0;
    Vector2 destinationPosition_;
};

} // namespace CardsFramework
