#pragma once

// BlackJackTable.hpp -- C++ port of UI/BlackJackTable.cs (XNA 4.0
// CardsStarterKit sample). Adds the per-place betting ring texture on top of
// the base GameTable.

#include <functional>
#include <string>

#include "../CardsFramework/GameTable.hpp"
#include "BlackjackCommon.hpp"

namespace Blackjack {

using CardsFramework::GameTable;

class BlackJackTable : public GameTable {
public:
    Texture2D RingTexture;
    Vector2 RingOffset;

    BlackJackTable(Vector2 ringOffset, Rectangle tableBounds, Vector2 dealerPosition, int places,
                   std::function<Vector2(int)> placeOrder, const std::string& theme, Game& game)
        : GameTable(tableBounds, dealerPosition, places, std::move(placeOrder), theme, game),
          RingOffset(ringOffset) {}

    void LoadContent() override {
        RingTexture = getGameProperty().getContentProperty().Load<Texture2D>("Images/UI/ring");
        GameTable::LoadContent();
    }

    void Draw(const GameTime& gameTime) override {
        GameTable::Draw(gameTime);

        TableSpriteBatch.Begin();
        for (int placeIndex = 0; placeIndex < Places; placeIndex++)
            TableSpriteBatch.Draw(RingTexture, PlaceOrder(placeIndex) + RingOffset, Color::White);
        TableSpriteBatch.End();
    }
};

} // namespace Blackjack
