#pragma once

// BlackjackAnimatedDealerHandComponent.hpp -- C++ port of
// UI/BlackjackAnimatedDealerHandComponent.cs (XNA 4.0 CardsStarterKit sample).

#include "../CardsFramework/AnimatedHandGameComponent.hpp"
#include "../CardsFramework/CardsGame.hpp"
#include "../CardsFramework/Hand.hpp"
#include "BlackjackCommon.hpp"

namespace Blackjack {

using CardsFramework::AnimatedHandGameComponent;
using CardsFramework::CardsGame;
using CardsFramework::Hand;

class BlackjackAnimatedDealerHandComponent : public AnimatedHandGameComponent {
public:
    BlackjackAnimatedDealerHandComponent(int place, Hand& hand, CardsGame& cardGame)
        : AnimatedHandGameComponent(place, hand, cardGame) {}

    Vector2 GetCardRelativePosition(int cardLocationInHand) const override {
        return Vector2(30.0f * cardLocationInHand, 0.0f);
    }
};

} // namespace Blackjack
