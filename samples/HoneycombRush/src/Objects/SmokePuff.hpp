#pragma once

// SmokePuff.hpp — C++ port of Objects/SmokePuff.cs (XNA 4.0 HoneycombRush
// sample). Represents a puff of smoke fired from the beekeeper's smoke gun;
// adds/removes itself from Game.Components as appropriate. Update()/Draw()
// are declared here but defined out-of-line in GameplayScreen.hpp — see
// missing.md.

#include "System/Random.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/TimeSpan.hpp"

#include "TexturedDrawableGameComponent.hpp"

namespace HoneycombRush {

// Represents a puff of smoke fired from the beekeeper's smoke gun. Port of
// Objects/SmokePuff.cs.
class SmokePuff : public TexturedDrawableGameComponent {
public:
    SmokePuff(Game& game, GameplayScreen* gamePlayScreen, Texture2D tex)
        : TexturedDrawableGameComponent(game, gamePlayScreen) {
        texture = std::move(tex);
        drawOrigin_ = Vector2((float)texture.getWidthProperty() / 2.0f, (float)texture.getHeightProperty() / 2.0f);
        setDrawOrderProperty(INT32_MAX - 15);
    }

    bool IsGone() const { return lifeTime_ <= System::TimeSpan::Zero; }

    Rectangle CentralCollisionArea() const override {
        int boundsWidth = (int)((float)texture.getWidthProperty() * spreadFactor_ * 1.5f);
        int boundsHeight = (int)((float)texture.getHeightProperty() * spreadFactor_ * 1.5f);
        return Rectangle((int)position.X - boundsWidth / 4, (int)position.Y - boundsHeight / 4, boundsWidth,
                          boundsHeight);
    }

    // Fires the smoke puff from a position at a velocity, adding it to the
    // game component collection.
    void Fire(Vector2 origin, Vector2 velocity) {
        spreadFactor_ = 0.05f;
        lifeTime_ = System::TimeSpan::FromSeconds(5);
        growthTimeTrack_ = System::TimeSpan::Zero;

        position = origin;
        velocity_ = velocity;
        initialVelocity_ = velocity;
        initialVelocity_.Normalize();

        acceleration_ = -(initialVelocity_)*6.0f;

        if (!isInGameComponents_) {
            getGameProperty().getComponentsProperty().Add(this);
            isInGameComponents_ = true;
        }
    }

    void Update(GameTime& gameTime) override;
    void Draw(const GameTime& gameTime) override;

    float SpreadFactor() const { return spreadFactor_; }
    Vector2 DrawOrigin() const { return drawOrigin_; }

private:
    static System::TimeSpan GrowthTimeInterval() { return System::TimeSpan::FromMilliseconds(50); }
    static constexpr float GrowthStep = 0.05f;

    System::TimeSpan lifeTime_;
    System::TimeSpan growthTimeTrack_;

    float spreadFactor_ = 0.0f;
    Vector2 initialVelocity_;
    Vector2 velocity_;
    Vector2 acceleration_;
    Vector2 drawOrigin_;

    System::Random random_;

    bool isInGameComponents_ = false;

    Vector2 GetRandomOffset() { return Vector2((float)(random_.Next(2) - 4), (float)(random_.Next(2) - 4)); }
};

} // namespace HoneycombRush
