#pragma once

// SwordSlash.hpp — C++ port of Elements/Specific/SwordSlash.cs (XNA 4.0
// NinjAcademy sample). Represents a sword slash effect on screen; takes
// care of enabling/disabling itself.

#include <cmath>
#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "System/TimeSpan.hpp"

#include "../TexturedDrawableGameComponent.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

// Port of Elements/Specific/SwordSlash.cs.
class SwordSlash : public TexturedDrawableGameComponent {
public:
    float Rotation = 0.0f;

    SwordSlash(Game& game, GameScreen* gameScreen, Texture2D texture)
        : TexturedDrawableGameComponent(game, gameScreen, std::move(texture)) {}

    float Stretch() const { return scaleVector_.Y; }
    void setStretch(float value) { scaleVector_.Y = value; }

    void Update(GameTime& gameTime) override {
        GameComponent::Update(gameTime);

        timer_ = timer_ + gameTime.getElapsedGameTimeProperty();

        switch (state_) {
            case State::Static:
                break;
            case State::Appearing:
                setStretch((float)(desiredScale_ * timer_.getTotalMillisecondsProperty() /
                                    growthDuration_.getTotalMillisecondsProperty()));
                if (timer_ >= growthDuration_) {
                    Fade(fadeDuration_);
                }
                break;
            case State::Fading:
                alpha_ = (float)(1.0 - timer_.getTotalMillisecondsProperty() / fadeDuration_.getTotalMillisecondsProperty());
                if (timer_ >= fadeDuration_) {
                    setEnabledProperty(false);
                    setVisibleProperty(false);
                }
                break;
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        spriteBatch_->Begin();
        spriteBatch_->Draw(texture_, source_, std::nullopt, mul(Color::White, alpha_), Rotation, textureOrigin_,
                           scaleVector_, SpriteEffects::None, 0.0f);
        spriteBatch_->End();
    }

    // Readies the sword slash to be displayed (enables/shows it).
    void Reset() {
        alpha_ = 1.0f;
        setEnabledProperty(true);
        setVisibleProperty(true);
    }

    // Fades the slash over fadeDuration; becomes inactive once done.
    void Fade(System::TimeSpan fadeDuration) {
        timer_ = System::TimeSpan::Zero;
        fadeDuration_ = fadeDuration;
        state_ = State::Fading;
    }

    // Positions the slash between two points; remains until changed.
    void PositionSlash(Vector2 source, Vector2 destination) {
        state_ = State::Static;
        source_ = source;

        InitializeSlashForCoordinates(source, destination);

        setStretch(desiredScale_);
    }

    // Displays a slash between two points, expanding then fading over the given durations.
    void Slash(Vector2 source, Vector2 destination, System::TimeSpan growthDuration, System::TimeSpan fadeDuration) {
        source_ = source;
        fadeDuration_ = fadeDuration;
        growthDuration_ = growthDuration;

        state_ = State::Appearing;
        setStretch(0.0f);

        InitializeSlashForCoordinates(source, destination);

        timer_ = System::TimeSpan::Zero;
    }

    // Initializes members for displaying a slash between two positions.
    void InitializeSlashForCoordinates(Vector2 source, Vector2 destination) {
        // Find the scale required to properly display the slash.
        desiredScale_ = (source - destination).Length() / (Bounds().Max.Y - Bounds().Min.Y);

        // Calculate the required rotation (flip Y since the screen's Y-axis is flipped).
        Vector2 desiredDirectionUnitVector = destination - source;
        desiredDirectionUnitVector.Y = -desiredDirectionUnitVector.Y;
        desiredDirectionUnitVector.Normalize();

        Rotation = (float)std::acos(Vector2::Dot(desiredDirectionUnitVector, Vector2::UnitY));

        if (desiredDirectionUnitVector.X < 0.0f) {
            Rotation = -Rotation;
        }
    }

private:
    enum class State { Static, Appearing, Fading };

    Vector2 textureOrigin_{6.0f, 75.0f};

    Vector2 source_;

    State state_ = State::Appearing;

    System::TimeSpan fadeDuration_ = System::TimeSpan::FromMilliseconds(500);
    System::TimeSpan growthDuration_ = System::TimeSpan::FromMilliseconds(100);
    System::TimeSpan timer_ = System::TimeSpan::Zero;

    float desiredScale_ = 0.0f;
    float alpha_ = 1.0f;

    Vector2 scaleVector_{1.0f, 1.0f};
};

} // namespace NinjAcademy
