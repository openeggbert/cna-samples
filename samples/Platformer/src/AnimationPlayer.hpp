#pragma once
#include <algorithm>
#include <optional>
#include <stdexcept>
#include "Animation.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

namespace Platformer {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

struct AnimationPlayer {
    Animation* animation_ = nullptr;
    int frameIndex_ = 0;
    float time_ = 0.0f;

    Animation* getAnimationProperty() const { return animation_; }
    int getFrameIndexProperty() const { return frameIndex_; }

    Vector2 getOriginProperty() const {
        return Vector2(animation_->getFrameWidthProperty() / 2.0f,
                       (float)animation_->getFrameHeightProperty());
    }

    void PlayAnimation(Animation* animation) {
        if (animation_ == animation) return;
        animation_ = animation;
        frameIndex_ = 0;
        time_ = 0.0f;
    }

    void Draw(GameTime& gameTime, SpriteBatch& spriteBatch, Vector2 position, SpriteEffects spriteEffects) {
        if (!animation_)
            throw std::runtime_error("No animation is currently playing.");

        time_ += (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        while (time_ > animation_->getFrameTimeProperty()) {
            time_ -= animation_->getFrameTimeProperty();
            if (animation_->getIsLoopingProperty())
                frameIndex_ = (frameIndex_ + 1) % animation_->getFrameCountProperty();
            else
                frameIndex_ = std::min(frameIndex_ + 1, animation_->getFrameCountProperty() - 1);
        }

        int frameSize = animation_->getTextureProperty().getHeightProperty();
        Rectangle source(frameIndex_ * frameSize, 0, frameSize, frameSize);

        spriteBatch.Draw(animation_->getTextureProperty(), position,
                         std::make_optional(source), Color(255, 255, 255, 255),
                         0.0f, getOriginProperty(), 1.0f, spriteEffects, 0.0f);
    }
};

} // namespace Platformer
