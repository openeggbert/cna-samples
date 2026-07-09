#pragma once

#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Point.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp>
#include <System/TimeSpan.hpp>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace Graphics3DSample {

// A sprite-sheet animation: frames laid out in a grid (sheetSize columns/rows,
// each frameSize pixels), advancing one frame every frameInterval and looping.
class Animation {
public:
    Point currentFrame;
    Point frameSize;

    Animation(Texture2D texture, Point size, Point sheetSize, System::TimeSpan interval)
        : frameSize(size), animationTexture_(std::move(texture)), sheetSize_(sheetSize),
          frameInterval_(interval) {}

    // Advances the animation; returns true if a new frame was reached.
    bool Update(const GameTime& gameTime) {
        bool progressed;

        if (nextFrame_ >= frameInterval_) {
            currentFrame.X++;
            if (currentFrame.X >= sheetSize_.X) {
                currentFrame.X = 0;
                currentFrame.Y++;
            }
            if (currentFrame.Y >= sheetSize_.Y) currentFrame.Y = 0;

            progressed = true;
            nextFrame_ = System::TimeSpan::Zero;
        } else {
            nextFrame_ = nextFrame_ + gameTime.getElapsedGameTimeProperty();
            progressed = false;
        }

        return progressed;
    }

    void Draw(SpriteBatch& spriteBatch, Vector2 position, float scale, SpriteEffects spriteEffect) {
        spriteBatch.Draw(animationTexture_, position,
                          Rectangle(frameSize.X * currentFrame.X, frameSize.Y * currentFrame.Y,
                                    frameSize.X, frameSize.Y),
                          Color::White, 0.0f, Vector2::Zero, scale, spriteEffect, 0.0f);
    }

private:
    Texture2D animationTexture_;
    Point sheetSize_;
    System::TimeSpan frameInterval_;
    System::TimeSpan nextFrame_;
};

} // namespace Graphics3DSample
