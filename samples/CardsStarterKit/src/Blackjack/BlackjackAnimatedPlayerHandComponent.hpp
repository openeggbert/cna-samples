#pragma once

// BlackjackAnimatedPlayerHandComponent.hpp -- C++ port of
// UI/BlackJackAnimatedPlayerHandComponent.cs (XNA 4.0 CardsStarterKit sample).

#include "../CardsFramework/AnimatedHandGameComponent.hpp"
#include "../CardsFramework/CardsGame.hpp"
#include "../CardsFramework/Hand.hpp"
#include "BlackjackCommon.hpp"

namespace Blackjack {

using CardsFramework::AnimatedHandGameComponent;
using CardsFramework::CardsGame;
using CardsFramework::Hand;

class BlackjackAnimatedPlayerHandComponent : public AnimatedHandGameComponent {
public:
    BlackjackAnimatedPlayerHandComponent(int place, Hand& hand, CardsGame& cardGame)
        : AnimatedHandGameComponent(place, hand, cardGame), offset_(Vector2::Zero) {}

    BlackjackAnimatedPlayerHandComponent(int place, Vector2 offset, Hand& hand, CardsGame& cardGame)
        : AnimatedHandGameComponent(place, hand, cardGame), offset_(offset) {}

    Vector2 GetCardRelativePosition(int cardLocationInHand) const override {
        return Vector2(25.0f * cardLocationInHand, -30.0f * cardLocationInHand) + offset_;
    }

private:
    Vector2 offset_;
};

} // namespace Blackjack
