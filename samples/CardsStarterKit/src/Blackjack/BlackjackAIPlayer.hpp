#pragma once

// BlackjackAIPlayer.hpp -- C++ port of Players/BlackjackAIPlayer.cs (XNA 4.0
// CardsStarterKit sample).

#include "System/EventArgs.hpp"
#include "System/EventHandler.hpp"
#include "System/Random.hpp"

#include "BlackjackCommon.hpp"
#include "BlackjackPlayer.hpp"

namespace Blackjack {

class BlackjackAIPlayer : public BlackjackPlayer {
public:
    System::EventHandler<System::EventArgs> Hit;
    System::EventHandler<System::EventArgs> Stand;

    BlackjackAIPlayer(const std::string& name, CardsFramework::CardsGame* game) : BlackjackPlayer(name, game) {}

    // Performs a move during a round.
    void AIPlay() {
        int value = FirstValue();
        if (FirstValueConsiderAce() && value + 10 <= 21)
            value += 10;

        if (value < 17 && !Hit.Empty())
            Hit.Raise(nullptr, System::EventArgs());
        else if (!Stand.Empty())
            Stand.Raise(nullptr, System::EventArgs());
    }

    // Returns the amount which the AI player decides to bet.
    int AIBet() {
        static const int chips[] = {0, 5, 25, 100, 500};
        int bet = chips[random_.Next(0, 5)];
        return bet < Balance ? bet : 0;
    }

private:
    System::Random random_;
};

} // namespace Blackjack
