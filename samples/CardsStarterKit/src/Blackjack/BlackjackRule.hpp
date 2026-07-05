#pragma once

// BlackjackRule.hpp -- C++ port of Rules/BlackjackRule.cs (XNA 4.0
// CardsStarterKit sample). Checks whether any player has achieved "blackjack".

#include <memory>
#include <vector>

#include "../CardsFramework/GameRule.hpp"
#include "../CardsFramework/Player.hpp"
#include "BlackjackCommon.hpp"
#include "BlackjackGameEventArgs.hpp"
#include "BlackjackPlayer.hpp"

namespace Blackjack {

class BlackJackRule : public CardsFramework::GameRule {
public:
    explicit BlackJackRule(const std::vector<std::shared_ptr<CardsFramework::Player>>& players) {
        for (auto& p : players)
            players_.push_back(std::static_pointer_cast<BlackjackPlayer>(p));
    }

    void Check() override {
        for (auto& player : players_) {
            player->CalculateValues();

            if (!player->BlackJack) {
                if (((player->FirstValue() == 21) ||
                     (player->FirstValueConsiderAce() && player->FirstValue() + 10 == 21)) &&
                    player->PlayerHand.Count() == 2) {
                    BlackjackGameEventArgs args;
                    args.Player = player.get();
                    args.Hand = HandTypes::First;
                    FireRuleMatch(args);
                }
            }
            if (!player->SecondBlackJack) {
                if (player->IsSplit &&
                    ((player->SecondValue() == 21) ||
                     (player->SecondValueConsiderAce() && player->SecondValue() + 10 == 21)) &&
                    player->SecondHand->Count() == 2) {
                    BlackjackGameEventArgs args;
                    args.Player = player.get();
                    args.Hand = HandTypes::Second;
                    FireRuleMatch(args);
                }
            }
        }
    }

private:
    std::vector<std::shared_ptr<BlackjackPlayer>> players_;
};

} // namespace Blackjack
