#pragma once

// AnimatedCardsGameComponent.hpp -- C++ port of UI/AnimatedCardsGameComponent.cs
// (XNA 4.0 CardsStarterKit sample). An AnimatedGameComponent for a single card.
// Update()/Draw() need CardsGame's full definition (cardsAssets/Theme/SpriteBatch),
// so they are defined out-of-line in CardsGame.hpp.

#include "AnimatedGameComponent.hpp"
#include "TraditionalCard.hpp"

namespace CardsFramework {

class AnimatedCardsGameComponent : public AnimatedGameComponent {
public:
    TraditionalCard* Card;

    AnimatedCardsGameComponent(TraditionalCard* card, CardsGame& cardGame)
        : AnimatedGameComponent(cardGame, nullptr), Card(card) {}

    void Update(GameTime& gameTime) override;
    void Draw(const GameTime& gameTime) override;
};

} // namespace CardsFramework
