#pragma once

// FramesetGameComponentAnimation.hpp -- C++ port of
// UI/FramesetGameComponentAnimation.cs (XNA 4.0 CardsStarterKit sample).
// A "typical" animation that alternates between a set of frames in a sheet.

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "AnimatedGameComponent.hpp"
#include "AnimatedGameComponentAnimation.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class FramesetGameComponentAnimation : public AnimatedGameComponentAnimation {
public:
    FramesetGameComponentAnimation(Texture2D* framesTexture, int numberOfFrames,
                                    int numberOfFramePerRow, Vector2 frameSize)
        : framesTexture_(framesTexture), numberOfFrames_(numberOfFrames),
          numberOfFramePerRow_(numberOfFramePerRow), frameSize_(frameSize) {}

    void Run(GameTime& gameTime) override {
        if (IsStarted()) {
            percent_ += (gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty() /
                         (Duration.getTotalMillisecondsProperty() / AnimationCycles())) * 100;

            if (percent_ >= 100)
                percent_ = 0;

            int animationIndex = (int)(numberOfFrames_ * percent_ / 100);
            Component->CurrentSegment = Rectangle(
                (int)frameSize_.X * (animationIndex % numberOfFramePerRow_),
                (int)frameSize_.Y * (animationIndex / numberOfFramePerRow_),
                (int)frameSize_.X, (int)frameSize_.Y);
            Component->CurrentFrame = framesTexture_;
        } else {
            Component->CurrentFrame = nullptr;
            Component->CurrentSegment.reset();
        }
        AnimatedGameComponentAnimation::Run(gameTime);
    }

private:
    Texture2D* framesTexture_;
    int numberOfFrames_;
    int numberOfFramePerRow_;
    Vector2 frameSize_;
    double percent_ = 0;
};

} // namespace CardsFramework
