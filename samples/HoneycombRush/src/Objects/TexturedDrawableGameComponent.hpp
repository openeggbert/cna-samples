#pragma once

// TexturedDrawableGameComponent.hpp — C++ port of
// Objects/TexturedDrawableGameComponent.cs (XNA 4.0 HoneycombRush sample).
// Abstract base for a component visually represented by a texture.

#include <string>
#include <unordered_map>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../Misc/Animation.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class GameplayScreen; // forward declaration

// Abstract class representing a component which has a texture that
// represents it visually. Port of Objects/TexturedDrawableGameComponent.cs.
class TexturedDrawableGameComponent : public DrawableGameComponent {
public:
    TexturedDrawableGameComponent(Game& game, GameplayScreen* gamePlayScreen)
        : DrawableGameComponent(game), gamePlayScreen(gamePlayScreen) {
        spriteBatch = game.getServicesProperty().GetService<SpriteBatch>();
    }

    virtual Rectangle Bounds() const {
        if (!texture.HasBackend())
            return Rectangle();
        return Rectangle((int)position.X, (int)position.Y, texture.getWidthProperty(), texture.getHeightProperty());
    }

    virtual Rectangle CentralCollisionArea() const { return Rectangle(); }

    std::unordered_map<std::string, Animation>* AnimationDefinitions = nullptr;

protected:
    SpriteBatch* spriteBatch = nullptr;
    Texture2D texture;
    Vector2 position;
    GameplayScreen* gamePlayScreen = nullptr;
};

} // namespace HoneycombRush
