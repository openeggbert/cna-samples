#pragma once

// FlipGameComponentAnimation.hpp -- C++ port of UI/FlipGameComponentAnimation.cs
// (XNA 4.0 CardsStarterKit sample). Makes the component appear as if flipped.

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "AnimatedGameComponent.hpp"
#include "AnimatedGameComponentAnimation.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class FlipGameComponentAnimation : public AnimatedGameComponentAnimation {
public:
    bool IsFromFaceDownToFaceUp = true;

    void Run(GameTime& gameTime) override {
        if (IsStarted()) {
            if (IsDone()) {
                Component->IsFaceDown = !IsFromFaceDownToFaceUp;
                Component->CurrentDestination.reset();
            } else {
                Texture2D* texture = Component->CurrentFrame;
                if (texture != nullptr) {
                    percent_ += (int)((gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty() /
                                        (Duration.getTotalMillisecondsProperty() / AnimationCycles())) * 100);

                    if (percent_ >= 100)
                        percent_ = 0;

                    int currentPercent;
                    if (percent_ < 50) {
                        currentPercent = percent_;
                        Component->IsFaceDown = IsFromFaceDownToFaceUp;
                    } else {
                        currentPercent = 100 - percent_;
                        Component->IsFaceDown = !IsFromFaceDownToFaceUp;
                    }

                    Component->CurrentDestination = Rectangle(
                        (int)(Component->CurrentPosition.X + texture->getWidthProperty() * currentPercent / 100),
                        (int)Component->CurrentPosition.Y,
                        (int)(texture->getWidthProperty() - texture->getWidthProperty() * currentPercent / 100 * 2),
                        texture->getHeightProperty());
                }
            }
        }
    }

private:
    int percent_ = 0;
};

} // namespace CardsFramework
