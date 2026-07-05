#pragma once

// TexturedDrawableGameComponent.hpp — C++ port of
// Elements/General/TexturedDrawableGameComponent.cs (XNA 4.0 NinjAcademy
// sample). Base class for a component which has a texture that represents
// it visually.

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "RestorableStateComponent.hpp"

namespace NinjAcademy {

class GameScreen; // forward declaration — stored but never dereferenced,
                   // matching the C# original's unused `gameScreen` field.

using Microsoft::Xna::Framework::BoundingBox;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Port of Elements/General/TexturedDrawableGameComponent.cs.
class TexturedDrawableGameComponent : public RestorableStateComponent {
public:
    Vector2 Position;

    TexturedDrawableGameComponent(Game& game, GameScreen* gameScreen, Texture2D texture)
        : RestorableStateComponent(game), gameScreen_(gameScreen), texture_(std::move(texture)),
          spriteBatch_(getGameProperty().getServicesProperty().GetService<SpriteBatch>()) {
        halfTextureDimensions_ = Vector2((float)texture_.getWidthProperty(), (float)texture_.getHeightProperty()) / 2.0f;
        VisualCenter = halfTextureDimensions_;
    }

    // Bounding box containing the component (actually a plane, as the box is
    // flat). Recalculated every call since Position may change.
    virtual BoundingBox Bounds() const {
        return BoundingBox(Vector3(Position, 0.0f),
                            Vector3(Position + Vector2((float)texture_.getWidthProperty(), (float)texture_.getHeightProperty()), 0.0f));
    }

    // The center of the component's bounds. Recalculated every call.
    virtual Vector2 Center() const { return Position + halfTextureDimensions_; }

    virtual float Width() const { return (float)texture_.getWidthProperty(); }
    virtual float Height() const { return (float)texture_.getHeightProperty(); }

protected:
    Vector2 VisualCenter;

    GameScreen* gameScreen_;
    Texture2D texture_;
    SpriteBatch* spriteBatch_;

private:
    Vector2 halfTextureDimensions_;
};

} // namespace NinjAcademy
