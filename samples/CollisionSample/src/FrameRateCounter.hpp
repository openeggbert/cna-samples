#pragma once
#include <string>
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"

namespace CollisionSample {

using namespace Microsoft::Xna::Framework;

// SpriteFont is not yet available in CNA, so FPS text rendering is omitted.
// The component still measures frame rate internally.
class FrameRateCounter : public DrawableGameComponent {
    int frameRate_    = 0;
    int frameCounter_ = 0;
    double elapsedTime_ = 0.0;

public:
    explicit FrameRateCounter(Game& game) : DrawableGameComponent(game) {}

    const std::string& GetTypeName() const override {
        static const std::string name = "FrameRateCounter";
        return name;
    }

    void Update(GameTime& gameTime) override {
        elapsedTime_ += gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        if (elapsedTime_ >= 1.0) {
            elapsedTime_ -= 1.0;
            frameRate_    = frameCounter_;
            frameCounter_ = 0;
        }
    }

    void Draw(const GameTime& /*gameTime*/) override {
        frameCounter_++;
        // FPS display omitted — SpriteFont not yet available in CNA.
    }
};

} // namespace CollisionSample
