#pragma once

// BustRule.hpp -- C++ port of Rules/BustRule.cs (XNA 4.0 CardsStarterKit
// sample). Checks whether any player has gone bust (hand value over 21).

#include <memory>
#include <vector>

#include "../CardsFramework/GameRule.hpp"
#include "../CardsFramework/Player.hpp"
#include "BlackjackCommon.hpp"
#include "BlackjackGameEventArgs.hpp"
#include "BlackjackPlayer.hpp"

namespace Blackjack {

class BustRule : public CardsFramework::GameRule {
public:
    explicit BustRule(const std::vector<std::shared_ptr<CardsFramework::Player>>& players) {
        for (auto& p : players)
            players_.push_back(std::static_pointer_cast<BlackjackPlayer>(p));
    }

    void Check() override {
        for (auto& player : players_) {
            player->CalculateValues();

            if (!player->Bust) {
                if (!player->FirstValueConsiderAce() && player->FirstValue() > 21) {
                    BlackjackGameEventArgs args;
                    args.Player = player.get();
                    args.Hand = HandTypes::First;
                    FireRuleMatch(args);
                }
            }
            if (!player->SecondBust) {
                if (player->IsSplit && !player->SecondValueConsiderAce() && player->SecondValue() > 21) {
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
