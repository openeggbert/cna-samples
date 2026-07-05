#pragma once

// UIUtility.hpp -- C++ port of Utils/UIUtilty.cs (XNA 4.0 CardsStarterKit sample).

#include <string>

#include "TraditionalCard.hpp"

namespace CardsFramework {

class UIUtility {
public:
    // Gets the name of a card asset (e.g. "ClubAce", "FirstJoker").
    static std::string GetCardAssetName(const TraditionalCard& card) {
        bool isJoker = card.Value == CardValue::FirstJoker || card.Value == CardValue::SecondJoker;

        std::string suitName;
        if (!isJoker) {
            switch (card.Type) {
                case CardSuit::Heart: suitName = "Heart"; break;
                case CardSuit::Diamond: suitName = "Diamond"; break;
                case CardSuit::Club: suitName = "Club"; break;
                case CardSuit::Spade: suitName = "Spade"; break;
                default: break;
            }
        }

        return suitName + CardValueName(card.Value);
    }

    static std::string CardValueName(CardValue value) {
        switch (value) {
            case CardValue::Ace: return "Ace";
            case CardValue::Two: return "Two";
            case CardValue::Three: return "Three";
            case CardValue::Four: return "Four";
            case CardValue::Five: return "Five";
            case CardValue::Six: return "Six";
            case CardValue::Seven: return "Seven";
            case CardValue::Eight: return "Eight";
            case CardValue::Nine: return "Nine";
            case CardValue::Ten: return "Ten";
            case CardValue::Jack: return "Jack";
            case CardValue::Queen: return "Queen";
            case CardValue::King: return "King";
            case CardValue::FirstJoker: return "FirstJoker";
            case CardValue::SecondJoker: return "SecondJoker";
            default: return "";
        }
    }
};

} // namespace CardsFramework
