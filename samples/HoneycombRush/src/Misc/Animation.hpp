#pragma once

// Animation.hpp — C++ port of Misc/Animation.cs (XNA 4.0 HoneycombRush sample).
// Supports sprite-sheet animation playback, including a "sub animation" mode
// (a fixed frame range played while the parent animation is otherwise idle).

#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/TimeSpan.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Supports animation playback. Port of Misc/Animation.cs.
class Animation {
public:
    Point currentFrame;
    Point frameSize;

    Vector2 Offset = Vector2::Zero;

    Animation() = default;

    Animation(Texture2D frameSheet, Point size, Point frameSheetSize)
        : frameSize(size), animatedCharacter_(std::move(frameSheet)), sheetSize_(frameSheetSize) {}

    [[nodiscard]] int FrameCount() const { return sheetSize_.X * sheetSize_.Y; }

    [[nodiscard]] bool IsActive() const { return isActive_; }

    [[nodiscard]] int FrameIndex() const { return sheetSize_.X * currentFrame.Y + currentFrame.X; }

    void setFrameIndex(int value) {
        currentFrame.Y = value / sheetSize_.X;
        currentFrame.X = value % sheetSize_.X;
    }

    void Update(const GameTime& gameTime, bool isInMotion) { Update(gameTime, isInMotion, false); }

    void Update(const GameTime& gameTime, bool isInMotion, bool runSubAnimation) {
        if (isActive_ && gameTime.getTotalGameTimeProperty() != lastestChangeTime_) {
            if (timeInterval_ != System::TimeSpan::Zero) {
                if (lastestChangeTime_ + timeInterval_ > gameTime.getTotalGameTimeProperty()) {
                    return;
                }
            }

            lastestChangeTime_ = gameTime.getTotalGameTimeProperty();
            if (FrameIndex() >= FrameCount()) {
                setFrameIndex(0);
            } else if (isInMotion) {
                if (runSubAnimation) {
                    if (lastSubFrame_ == -1) {
                        lastSubFrame_ = startFrame_;
                    }

                    currentFrame.Y = lastSubFrame_ / sheetSize_.X;
                    currentFrame.X = lastSubFrame_ % sheetSize_.X;

                    lastSubFrame_ += 1;
                    if (lastSubFrame_ > endFrame_) {
                        lastSubFrame_ = startFrame_;
                    }
                } else {
                    if (drawWasAlreadyCalledOnce_) {
                        currentFrame.X++;
                        if (currentFrame.X >= sheetSize_.X) {
                            currentFrame.X = 0;
                            currentFrame.Y++;
                        }
                        if (currentFrame.Y >= sheetSize_.Y)
                            currentFrame.Y = 0;

                        if (lastSubFrame_ != -1) {
                            lastSubFrame_ = -1;
                        }
                    }
                }
            }
        }
    }

    void Draw(SpriteBatch& spriteBatch, Vector2 position, SpriteEffects spriteEffect) {
        Draw(spriteBatch, position, 1.0f, spriteEffect);
    }

    void Draw(SpriteBatch& spriteBatch, Vector2 position, float scale, SpriteEffects spriteEffect) {
        drawWasAlreadyCalledOnce_ = true;

        spriteBatch.Draw(animatedCharacter_, position + Offset,
                          Rectangle(frameSize.X * currentFrame.X, frameSize.Y * currentFrame.Y, frameSize.X,
                                    frameSize.Y),
                          Color::White, 0.0f, Vector2::Zero, scale, spriteEffect, 0.0f);
    }

    void PlayFromFrameIndex(int frameIndex) {
        setFrameIndex(frameIndex);
        isActive_ = true;
        drawWasAlreadyCalledOnce_ = false;
    }

    void SetSubAnimation(int startFrame, int endFrame) {
        startFrame_ = startFrame;
        endFrame_ = endFrame;
    }

    void SetFrameInterval(System::TimeSpan interval) { timeInterval_ = interval; }

private:
    Texture2D animatedCharacter_;
    Point sheetSize_;

    System::TimeSpan lastestChangeTime_;
    System::TimeSpan timeInterval_ = System::TimeSpan::Zero;

    int startFrame_ = 0;
    int endFrame_ = 0;
    int lastSubFrame_ = -1;

    bool drawWasAlreadyCalledOnce_ = false;
    bool isActive_ = false;
};

} // namespace HoneycombRush
