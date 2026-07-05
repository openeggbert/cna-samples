#pragma once

// CardPacket.hpp -- C++ port of Cards/CardPacket.cs (XNA 4.0 CardsStarterKit
// sample). A packet of cards; may lose cards or deal them to a Hand, but may
// not receive new cards unless derived (see Hand.hpp).
//
// XNA's TraditionalCard is a reference type: the same object instance keeps
// its identity as it moves between CardPacket/Hand instances (comparisons
// elsewhere in the framework rely on this, e.g. finding a card's animated
// component by pointer equality). This port stores cards as
// std::unique_ptr<TraditionalCard> and *moves* the pointer between packets/
// hands rather than copying the TraditionalCard value, preserving that
// identity guarantee -- and matching the C# doc comment that "a card may
// only be held by one CardPacket at any given time" exactly.

#include <memory>
#include <vector>

#include "System/EventArgs.hpp"
#include "System/EventHandler.hpp"
#include "System/Random.hpp"

#include "TraditionalCard.hpp"

namespace CardsFramework {

class Hand;  // forward declaration (DealCardToHand/DealCardsToHand defined in Hand.hpp)

// Card-related event args holding event information of a TraditionalCard.
class CardEventArgs : public System::EventArgs {
public:
    TraditionalCard* Card = nullptr;
};

class CardPacket {
public:
    // An event which triggers when a card is removed from the collection.
    System::EventHandler<CardEventArgs> LostCard;

    int Count() const { return (int)cards_.size(); }

    TraditionalCard& operator[](int index) { return *cards_[index]; }
    const TraditionalCard& operator[](int index) const { return *cards_[index]; }

    // Initializes a new instance, adding the requested decks/jokers/suits/values.
    CardPacket(int numberOfDecks, int jokersInDeck, CardSuit suits, CardValue cardValues) {
        for (int deckIndex = 0; deckIndex < numberOfDecks; deckIndex++) {
            AddSuit(suits, cardValues);

            for (int j = 0; j < jokersInDeck / 2; j++) {
                cards_.push_back(std::make_unique<TraditionalCard>(CardSuit::Club, CardValue::FirstJoker, this));
                cards_.push_back(std::make_unique<TraditionalCard>(CardSuit::Club, CardValue::SecondJoker, this));
            }

            if (jokersInDeck % 2 == 1) {
                cards_.push_back(std::make_unique<TraditionalCard>(CardSuit::Club, CardValue::FirstJoker, this));
            }
        }
    }

    virtual ~CardPacket() = default;

    // Shuffles the cards in the packet by randomly changing card placement.
    void Shuffle() {
        System::Random random;
        std::vector<std::unique_ptr<TraditionalCard>> shuffledDeck;

        while (!cards_.empty()) {
            int index = random.Next(0, (int)cards_.size());
            shuffledDeck.push_back(std::move(cards_[index]));
            cards_.erase(cards_.begin() + index);
        }

        cards_ = std::move(shuffledDeck);
    }

    // Removes the specified card from the packet. The first matching card is
    // removed. May only be performed internally by other card-framework
    // classes, matching the C# `internal` accessibility.
    std::unique_ptr<TraditionalCard> Remove(TraditionalCard* card) {
        for (size_t i = 0; i < cards_.size(); i++) {
            if (cards_[i].get() == card) {
                std::unique_ptr<TraditionalCard> removed = std::move(cards_[i]);
                cards_.erase(cards_.begin() + i);

                CardEventArgs args;
                args.Card = removed.get();
                LostCard.Raise(nullptr, args);

                return removed;
            }
        }
        return nullptr;
    }

    // Deals the first card from the collection to a specified hand.
    // Defined out-of-line in Hand.hpp (needs Hand::Add).
    TraditionalCard& DealCardToHand(Hand& destinationHand);

    // Deals several cards to a specified hand.
    // Defined out-of-line in Hand.hpp.
    void DealCardsToHand(Hand& destinationHand, int count);

protected:
    CardPacket() = default;

    std::vector<std::unique_ptr<TraditionalCard>> cards_;

private:
    void AddSuit(CardSuit suits, CardValue cardValues) {
        if ((suits & CardSuit::Club) == CardSuit::Club) AddCards(CardSuit::Club, cardValues);
        if ((suits & CardSuit::Diamond) == CardSuit::Diamond) AddCards(CardSuit::Diamond, cardValues);
        if ((suits & CardSuit::Heart) == CardSuit::Heart) AddCards(CardSuit::Heart, cardValues);
        if ((suits & CardSuit::Spade) == CardSuit::Spade) AddCards(CardSuit::Spade, cardValues);
    }

    void AddCards(CardSuit suit, CardValue cardValues) {
        static const CardValue kAll[] = {
            CardValue::Ace, CardValue::Two, CardValue::Three, CardValue::Four, CardValue::Five,
            CardValue::Six, CardValue::Seven, CardValue::Eight, CardValue::Nine, CardValue::Ten,
            CardValue::Jack, CardValue::Queen, CardValue::King,
        };
        for (CardValue v : kAll) {
            if ((cardValues & v) == v)
                cards_.push_back(std::make_unique<TraditionalCard>(suit, v, this));
        }
    }
};

} // namespace CardsFramework
