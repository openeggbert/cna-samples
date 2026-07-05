#pragma once

// TraditionalCard.hpp -- C++ port of Cards/TraditionalCard.cs (XNA 4.0
// CardsStarterKit sample).

#include <stdexcept>

namespace CardsFramework {

class CardPacket;  // forward declaration
class Hand;        // forward declaration

// Enum defining the various types of cards for a traditional-western card-set.
enum class CardSuit : unsigned {
    Heart = 0x01,
    Diamond = 0x02,
    Club = 0x04,
    Spade = 0x08,
    AllSuits = Heart | Diamond | Club | Spade,
};

[[nodiscard]] constexpr CardSuit operator|(CardSuit left, CardSuit right) {
    return static_cast<CardSuit>(static_cast<unsigned>(left) | static_cast<unsigned>(right));
}
[[nodiscard]] constexpr CardSuit operator&(CardSuit left, CardSuit right) {
    return static_cast<CardSuit>(static_cast<unsigned>(left) & static_cast<unsigned>(right));
}

// Enum defining the various types of card values for a traditional-western card-set.
enum class CardValue : unsigned {
    Ace = 0x01,
    Two = 0x02,
    Three = 0x04,
    Four = 0x08,
    Five = 0x10,
    Six = 0x20,
    Seven = 0x40,
    Eight = 0x80,
    Nine = 0x100,
    Ten = 0x200,
    Jack = 0x400,
    Queen = 0x800,
    King = 0x1000,
    FirstJoker = 0x2000,
    SecondJoker = 0x4000,
    AllNumbers = 0x3FF,
    NonJokers = 0x1FFF,
    Jokers = FirstJoker | SecondJoker,
    AllFigures = Jack | Queen | King,
};

[[nodiscard]] constexpr CardValue operator|(CardValue left, CardValue right) {
    return static_cast<CardValue>(static_cast<unsigned>(left) | static_cast<unsigned>(right));
}
[[nodiscard]] constexpr CardValue operator&(CardValue left, CardValue right) {
    return static_cast<CardValue>(static_cast<unsigned>(left) & static_cast<unsigned>(right));
}

// Traditional-western card. Each card has a defined Type/Value and the
// CardPacket in which it is being held. A card may not be held in more than
// one CardPacket -- transfers only happen through MoveToHand(), which is
// defined out-of-line in Hand.hpp once Hand/CardPacket are complete types.
class TraditionalCard {
public:
    CardSuit Type;
    CardValue Value;
    CardPacket* HoldingCardCollection;

    TraditionalCard(CardSuit type, CardValue value, CardPacket* holdingCardCollection)
        : Type(type), Value(value), HoldingCardCollection(holdingCardCollection) {
        switch (type) {
            case CardSuit::Club:
            case CardSuit::Diamond:
            case CardSuit::Heart:
            case CardSuit::Spade:
                break;
            default:
                throw std::invalid_argument("type must be single value");
        }

        switch (value) {
            case CardValue::Ace:
            case CardValue::Two:
            case CardValue::Three:
            case CardValue::Four:
            case CardValue::Five:
            case CardValue::Six:
            case CardValue::Seven:
            case CardValue::Eight:
            case CardValue::Nine:
            case CardValue::Ten:
            case CardValue::Jack:
            case CardValue::Queen:
            case CardValue::King:
            case CardValue::FirstJoker:
            case CardValue::SecondJoker:
                break;
            default:
                throw std::invalid_argument("value must be single value");
        }
    }

    // Moves the card from its current CardPacket to the specified hand.
    // Defined out-of-line in Hand.hpp (needs Hand::Add / CardPacket::Remove).
    void MoveToHand(Hand& hand);
};

} // namespace CardsFramework
