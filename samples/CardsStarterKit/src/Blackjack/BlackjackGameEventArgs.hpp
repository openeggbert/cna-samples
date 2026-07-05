#pragma once

// BlackjackGameEventArgs.hpp -- C++ port of Rules/BlackjackGameEventArgs.cs
// (XNA 4.0 CardsStarterKit sample).

#include "System/EventArgs.hpp"

#include "../CardsFramework/Player.hpp"
#include "BlackjackCommon.hpp"

namespace Blackjack {

enum class HandTypes {
    First,
    Second,
};

class BlackjackGameEventArgs : public System::EventArgs {
public:
    CardsFramework::Player* Player = nullptr;
    HandTypes Hand = HandTypes::First;
};

} // namespace Blackjack
