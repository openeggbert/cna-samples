#pragma once

// DisappearingAnimationComponent.hpp — C++ port of
// Elements/General/EndingAnimationComponent.cs (XNA 4.0 NinjAcademy sample;
// the file is named EndingAnimationComponent.cs but the class inside is
// DisappearingAnimationComponent). A component that disappears once its
// animation becomes inactive -- used for the explosion effect.

#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

#include "AnimatedComponent.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Graphics::SpriteEffects;

// Port of Elements/General/EndingAnimationComponent.cs's DisappearingAnimationComponent.
class DisappearingAnimationComponent : public AnimatedComponent {
public:
    DisappearingAnimationComponent(Game& game, GameScreen* gameScreen, Animation animation)
        : AnimatedComponent(game, gameScreen, std::move(animation)) {}

    void Update(GameTime& gameTime) override {
        AnimatedComponent::Update(gameTime);

        if (!animation_.IsActive) {
            setEnabledProperty(false);
            setVisibleProperty(false);
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        spriteBatch_->Begin();
        animation_.Draw(*spriteBatch_, Position, 0.0f, VisualCenter, 1.0f, SpriteEffects::None, 0.0f);
        spriteBatch_->End();
    }

    // Shows the animation at a specified position, restarting it from frame 0.
    void Show(Vector2 position) {
        Position = position;
        animation_.PlayFromFrameIndex(0);
        setEnabledProperty(true);
        setVisibleProperty(true);
    }
};

} // namespace NinjAcademy
