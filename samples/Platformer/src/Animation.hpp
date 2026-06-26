#pragma once
#include <optional>
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace Platformer {

class Animation {
    std::optional<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
    float frameTime_ = 0.0f;
    bool isLooping_ = false;

public:
    Animation() = default;

    Animation(Microsoft::Xna::Framework::Graphics::Texture2D texture, float frameTime, bool isLooping)
        : texture_(std::move(texture)), frameTime_(frameTime), isLooping_(isLooping) {}

    const Microsoft::Xna::Framework::Graphics::Texture2D& getTextureProperty() const { return *texture_; }
    float getFrameTimeProperty() const { return frameTime_; }
    bool getIsLoopingProperty() const { return isLooping_; }
    int getFrameWidthProperty() const { return texture_->getHeightProperty(); } // square frames
    int getFrameHeightProperty() const { return texture_->getHeightProperty(); }
    int getFrameCountProperty() const { return texture_->getWidthProperty() / getFrameWidthProperty(); }
};

} // namespace Platformer
