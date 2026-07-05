#pragma once

// Hand.hpp -- C++ port of Cards/Hand.cs (XNA 4.0 CardsStarterKit sample).
// Represents a hand of cards held by a player, dealer, or the game table.
// A Hand is a CardPacket that may also receive cards from any CardPacket or
// another Hand.

#include <memory>

#include "CardPacket.hpp"
#include "TraditionalCard.hpp"

namespace CardsFramework {

class Hand : public CardPacket {
public:
    // An event which triggers when a card is added to the hand.
    System::EventHandler<CardEventArgs> ReceivedCard;

    Hand() : CardPacket() {}

    // Adds the specified card to the hand, as the last card of the hand.
    // `internal` in the original -- CardsFramework-only callers here too.
    void Add(std::unique_ptr<TraditionalCard> card) {
        CardEventArgs args;
        args.Card = card.get();
        cards_.push_back(std::move(card));
        ReceivedCard.Raise(nullptr, args);
    }
};

// ---- Out-of-line definitions needing both CardPacket and Hand complete ----

inline void TraditionalCard::MoveToHand(Hand& hand) {
    std::unique_ptr<TraditionalCard> self = HoldingCardCollection->Remove(this);
    HoldingCardCollection = &hand;
    hand.Add(std::move(self));
}

inline TraditionalCard& CardPacket::DealCardToHand(Hand& destinationHand) {
    TraditionalCard* firstCard = cards_.front().get();
    firstCard->MoveToHand(destinationHand);
    return *firstCard;
}

inline void CardPacket::DealCardsToHand(Hand& destinationHand, int count) {
    for (int cardIndex = 0; cardIndex < count; cardIndex++) {
        DealCardToHand(destinationHand);
    }
}

} // namespace CardsFramework
