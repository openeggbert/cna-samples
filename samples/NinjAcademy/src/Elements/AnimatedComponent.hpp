#pragma once

// AnimatedComponent.hpp — C++ port of Elements/General/AnimatedComponent.cs
// (XNA 4.0 NinjAcademy sample). Base class for a component which has an
// animation that represents it visually.

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/GameComponent.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include "../Animation.hpp"
#include "TexturedDrawableGameComponent.hpp"

namespace NinjAcademy {

// Port of Elements/General/AnimatedComponent.cs.
class AnimatedComponent : public TexturedDrawableGameComponent {
public:
    AnimatedComponent(Game& game, GameScreen* gameScreen, Animation animation)
        : TexturedDrawableGameComponent(game, gameScreen, animation.AnimationSheet()), animation_(std::move(animation)) {
        VisualCenter = animation_.VisualCenter();
        halfFrameSize_ = Vector2((float)animation_.FrameWidth(), (float)animation_.FrameHeight()) / 2.0f;
    }

    // Creates a new animated component instance with a single-frame animation.
    AnimatedComponent(Game& game, GameScreen* gameScreen, Texture2D texture)
        : AnimatedComponent(game, gameScreen,
                            Animation(texture, Point(texture.getWidthProperty(), texture.getHeightProperty()), Point(1, 1),
                                       Vector2((float)texture.getWidthProperty(), (float)texture.getHeightProperty()) / 2.0f,
                                       false)) {}

    BoundingBox Bounds() const override {
        return BoundingBox(Vector3(Position, 0.0f),
                            Vector3(Position + Vector2((float)animation_.FrameWidth(), (float)animation_.FrameHeight()), 0.0f));
    }

    float Width() const override { return (float)animation_.FrameWidth(); }
    float Height() const override { return (float)animation_.FrameHeight(); }
    Vector2 Center() const override { return Position + halfFrameSize_; }

    void Update(GameTime& gameTime) override {
        GameComponent::Update(gameTime);
        animation_.Update(gameTime);
    }

    // Resets the associated animation.
    void ResetAnimation() { animation_.PlayFromFrameIndex(0); }

    // Pauses/resumes the component's animation.
    void Pause() { animation_.IsActive = false; }
    void Resume() { animation_.IsActive = true; }

protected:
    Animation animation_;

private:
    Vector2 halfFrameSize_;
};

} // namespace NinjAcademy
