#pragma once

// AnimatingSprite.hpp -- C++ port of RolePlayingGameData/Animation/AnimatingSprite.cs.

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "Animation.hpp"
#include "ContentObject.hpp"
#include "Direction.hpp"

namespace RolePlayingGameData {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// A sprite sheet with flipbook-style animations.
class AnimatingSprite : public ContentObject {
public:
    std::string TextureName;
    std::shared_ptr<Texture2D> Texture;

    Point FrameDimensions() const { return frameDimensions_; }
    void SetFrameDimensions(Point v) {
        frameDimensions_ = v;
        frameOrigin_ = Point(v.X / 2, v.Y / 2);
    }

    int FramesPerRow = 0;
    Vector2 SourceOffset;

    std::vector<std::shared_ptr<Animation>> Animations;

    // Enumerate the animations on this animated sprite by name (case-insensitive).
    std::shared_ptr<Animation> Find(const std::string& animationName) const {
        if (animationName.empty()) return nullptr;
        for (auto& a : Animations) {
            if (a->Name.size() == animationName.size() &&
                std::equal(a->Name.begin(), a->Name.end(), animationName.begin(),
                           [](char x, char y) { return std::tolower((unsigned char)x) == std::tolower((unsigned char)y); }))
                return a;
        }
        return nullptr;
    }

    bool AddAnimation(const std::shared_ptr<Animation>& animation) {
        if (animation && !Find(animation->Name)) {
            Animations.push_back(animation);
            return true;
        }
        return false;
    }

    Rectangle SourceRectangle() const { return sourceRectangle_; }

    void PlayAnimation(const std::shared_ptr<Animation>& animation) {
        if (animation != currentAnimation_) {
            currentAnimation_ = animation;
            ResetAnimation();
        }
    }
    void PlayAnimation(int index) { PlayAnimation(Animations.at(index)); }
    void PlayAnimation(const std::string& name) { PlayAnimation(Find(name)); }
    void PlayAnimation(const std::string& name, Direction direction) {
        PlayAnimation(name + ToString(direction));
    }

    void ResetAnimation() {
        elapsedTime_ = 0.0f;
        if (currentAnimation_) {
            currentFrame_ = currentAnimation_->StartingFrame;
            UpdateAnimation(0.0f);
        }
    }

    void AdvanceToEnd() {
        if (currentAnimation_) {
            currentFrame_ = currentAnimation_->EndingFrame;
            UpdateAnimation(0.0f);
        }
    }

    void StopAnimation() { currentAnimation_ = nullptr; }

    bool IsPlaybackComplete() const {
        return !currentAnimation_ || (!currentAnimation_->IsLoop && currentFrame_ > currentAnimation_->EndingFrame);
    }

    void UpdateAnimation(float elapsedSeconds) {
        if (IsPlaybackComplete()) return;

        if (currentAnimation_->IsLoop && currentFrame_ > currentAnimation_->EndingFrame) {
            currentFrame_ = currentAnimation_->StartingFrame;
        }

        int column = (currentFrame_ - 1) / FramesPerRow;
        sourceRectangle_ = Rectangle(
            (currentFrame_ - 1 - (column * FramesPerRow)) * frameDimensions_.X,
            column * frameDimensions_.Y, frameDimensions_.X, frameDimensions_.Y);

        elapsedTime_ += elapsedSeconds;
        while (elapsedTime_ * 1000.0f > (float)currentAnimation_->Interval) {
            currentFrame_++;
            elapsedTime_ -= (float)currentAnimation_->Interval / 1000.0f;
        }
    }

    void Draw(SpriteBatch& spriteBatch, const Vector2& position, float layerDepth) const {
        Draw(spriteBatch, position, layerDepth, SpriteEffects::None);
    }
    void Draw(SpriteBatch& spriteBatch, const Vector2& position, float layerDepth, SpriteEffects effect) const {
        if (Texture) {
            spriteBatch.Draw(*Texture, position, sourceRectangle_, Microsoft::Xna::Framework::Color(255, 255, 255, 255),
                              0.0f, SourceOffset, 1.0f, effect,
                              Microsoft::Xna::Framework::MathHelper::Clamp(layerDepth, 0.0f, 1.0f));
        }
    }

    std::shared_ptr<AnimatingSprite> Clone() const {
        auto sprite = std::make_shared<AnimatingSprite>();
        sprite->Animations = Animations;
        sprite->currentAnimation_ = currentAnimation_;
        sprite->currentFrame_ = currentFrame_;
        sprite->elapsedTime_ = elapsedTime_;
        sprite->frameDimensions_ = frameDimensions_;
        sprite->frameOrigin_ = frameOrigin_;
        sprite->FramesPerRow = FramesPerRow;
        sprite->SourceOffset = SourceOffset;
        sprite->sourceRectangle_ = sourceRectangle_;
        sprite->Texture = Texture;
        sprite->TextureName = TextureName;
        return sprite;
    }

private:
    Point frameDimensions_;
    Point frameOrigin_;
    std::shared_ptr<Animation> currentAnimation_;
    int currentFrame_ = 0;
    float elapsedTime_ = 0.0f;
    Rectangle sourceRectangle_;
};

} // namespace RolePlayingGameData
