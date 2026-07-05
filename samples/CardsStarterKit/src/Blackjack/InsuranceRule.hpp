#pragma once

// InsuranceRule.hpp -- C++ port of Rules/InsuranceRule.cs (XNA 4.0
// CardsStarterKit sample). Checks whether the human player can use insurance
// (dealer's revealed card is an ace).

#include "../CardsFramework/GameRule.hpp"
#include "../CardsFramework/Hand.hpp"
#include "../CardsFramework/TraditionalCard.hpp"
#include "BlackjackCommon.hpp"

namespace Blackjack {

class InsuranceRule : public CardsFramework::GameRule {
public:
    explicit InsuranceRule(CardsFramework::Hand& dealerHand) : dealerHand_(dealerHand) {}

    void Check() override {
        if (!done_) {
            if (dealerHand_.Count() > 0) {
                if (dealerHand_[0].Value == CardsFramework::CardValue::Ace) {
                    FireRuleMatch(System::EventArgs());
                }
                done_ = true;
            }
        }
    }

private:
    CardsFramework::Hand& dealerHand_;
    bool done_ = false;
};

} // namespace Blackjack
