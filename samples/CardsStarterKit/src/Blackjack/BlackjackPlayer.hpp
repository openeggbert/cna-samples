#pragma once

// BlackjackPlayer.hpp -- C++ port of Players/BlackjackPlayer.cs (XNA 4.0
// CardsStarterKit sample). Note: the base Player::Hand field was renamed to
// PlayerHand in this port to avoid clashing with the C++ Hand type name
// (see CardsFramework/Player.hpp and missing.md).

#include <optional>
#include <stdexcept>

#include "../CardsFramework/CardsGame.hpp"
#include "../CardsFramework/Hand.hpp"
#include "../CardsFramework/Player.hpp"
#include "BlackjackCommon.hpp"
#include "BlackjackGameEventArgs.hpp"

namespace Blackjack {

using CardsFramework::CardValue;
using CardsFramework::Hand;
using CardsFramework::Player;

class BlackjackPlayer : public Player {
public:
    bool Bust = false;
    bool SecondBust = false;
    bool BlackJack = false;
    bool SecondBlackJack = false;
    bool Double = false;
    bool SecondDouble = false;

    bool IsSplit = false;
    std::optional<Hand> SecondHand;

    HandTypes CurrentHandType = HandTypes::First;

    // Returns the hand the player is currently interacting with.
    Hand& CurrentHand() {
        switch (CurrentHandType) {
            case HandTypes::First: return PlayerHand;
            case HandTypes::Second: return *SecondHand;
            default: throw std::runtime_error("No hand to return");
        }
    }

    int FirstValue() const { return firstValue_; }
    bool FirstValueConsiderAce() const { return firstValueConsiderAce_; }
    int SecondValue() const { return secondValue_; }
    bool SecondValueConsiderAce() const { return secondValueConsiderAce_; }

    bool MadeBet() const { return BetAmount > 0; }
    bool IsDoneBetting = false;
    float Balance = 500;
    float BetAmount = 0;
    bool IsInsurance = false;

    BlackjackPlayer(const std::string& name, CardsFramework::CardsGame* game) : Player(name, game) {}
    ~BlackjackPlayer() override = default;

    // Bets a specified amount, if the balance permits. Returns whether the
    // bet succeeded; BetAmount/Balance are only updated on success.
    bool Bet(float amount) {
        if (amount > Balance)
            return false;
        BetAmount += amount;
        Balance -= amount;
        return true;
    }

    // Resets the player's bet to 0, returning the current bet to the balance.
    void ClearBet() {
        Balance += BetAmount;
        BetAmount = 0;
    }

    // Calculates the values of the player's two hands.
    void CalculateValues() {
        CalculateValue(PlayerHand, *Game, firstValue_, firstValueConsiderAce_);
        if (SecondHand.has_value())
            CalculateValue(*SecondHand, *Game, secondValue_, secondValueConsiderAce_);
    }

    // Resets the player's various state fields.
    void ResetValues() {
        BlackJack = false;
        SecondBlackJack = false;
        Bust = false;
        SecondBust = false;
        Double = false;
        SecondDouble = false;
        firstValue_ = 0;
        firstValueConsiderAce_ = false;
        IsSplit = false;
        secondValue_ = 0;
        secondValueConsiderAce_ = false;
        BetAmount = 0;
        IsDoneBetting = false;
        IsInsurance = false;
        CurrentHandType = HandTypes::First;
    }

    // Initializes the player's second hand.
    void InitializeSecondHand() { SecondHand.emplace(); }

    // Splits the current hand into two, per blackjack rules.
    void SplitHand() {
        if (!SecondHand.has_value())
            throw std::logic_error("Second hand is not initialized.");
        if (IsSplit)
            throw std::logic_error("A hand cannot be split more than once.");
        if (PlayerHand.Count() != 2)
            throw std::logic_error("You must have two cards to perform a split.");
        if (PlayerHand[0].Value != PlayerHand[1].Value)
            throw std::logic_error("You can only split when both cards are of identical value.");

        IsSplit = true;
        PlayerHand[1].MoveToHand(*SecondHand);
    }

private:
    static void CalculateValue(Hand& hand, CardsFramework::CardsGame& game, int& value, bool& considerAce) {
        value = 0;
        considerAce = false;

        for (int cardIndex = 0; cardIndex < hand.Count(); cardIndex++) {
            value += game.CardValue(hand[cardIndex]);
            if (hand[cardIndex].Value == CardValue::Ace)
                considerAce = true;
        }

        if (considerAce && value + 10 > 21)
            considerAce = false;
    }

    int firstValue_ = 0;
    bool firstValueConsiderAce_ = false;
    int secondValue_ = 0;
    bool secondValueConsiderAce_ = false;
};

} // namespace Blackjack
