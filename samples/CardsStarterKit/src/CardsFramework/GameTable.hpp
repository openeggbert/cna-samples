#pragma once

// GameTable.hpp -- C++ port of UI/GameTable.cs (XNA 4.0 CardsStarterKit sample).
// The UI representation of the table where the game is played.

#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class GameTable : public DrawableGameComponent {
public:
    std::string Theme;
    Texture2D TableTexture;
    Vector2 DealerPosition;
    SpriteBatch TableSpriteBatch;
    std::function<Vector2(int)> PlaceOrder;
    Rectangle TableBounds;
    int Places;

    // Returns the player position on the table according to the player index
    // (relative to the entire game area, even if the table only occupies
    // part of it).
    Vector2 operator[](int index) const {
        return Vector2((float)TableBounds.X, (float)TableBounds.Y) + PlaceOrder(index);
    }

    GameTable(Rectangle tableBounds, Vector2 dealerPosition, int places,
              std::function<Vector2(int)> placeOrder, const std::string& theme, Game& game)
        : DrawableGameComponent(game), Theme(theme),
          DealerPosition(dealerPosition + Vector2((float)tableBounds.X, (float)tableBounds.Y)),
          TableSpriteBatch(game.getGraphicsDeviceProperty()), PlaceOrder(std::move(placeOrder)),
          TableBounds(tableBounds), Places(places) {}

    // Load the table texture.
    void LoadContent() override {
        TableTexture = getGameProperty().getContentProperty().Load<Texture2D>("Images/UI/table");
        DrawableGameComponent::LoadContent();
    }

    // Render the table.
    void Draw(const GameTime& gameTime) override {
        TableSpriteBatch.Begin();
        TableSpriteBatch.Draw(TableTexture, TableBounds, Color::White);
        TableSpriteBatch.End();
        DrawableGameComponent::Draw(gameTime);
    }
};

} // namespace CardsFramework
