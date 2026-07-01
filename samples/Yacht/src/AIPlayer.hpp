#pragma once

#include <string>

#include "System/Random.hpp"

#include "YachtPlayer.hpp"
#include "DiceHandler.hpp"

namespace Yacht {

// Possible states for an AI player's turn-taking state machine.
enum class AIState {
    Roll,
    Rolling,
    ChooseDice,
    SelectScore,
    WriteScore,
};

// An AI player. Ported near-verbatim from Objects/AIPlayer.cs -- it was
// already fully self-contained in the original (no NetworkManager
// dependency), so this is the most faithful of the three player ports.
//
// PerformPlayerLogic() is declared here but *defined* out-of-line at the
// bottom of GameStateHandler.hpp, since almost every state in the machine
// calls back into GameStateHandler (see HumanPlayer.hpp for the same
// cycle-break idiom, applied here for the same reason).
class AIPlayer : public YachtPlayer {
public:
    AIPlayer(const std::string& name, DiceHandler* diceHandler)
        : YachtPlayer(name, diceHandler) {}

    AIState State = AIState::Roll;

    void PerformPlayerLogic() override;

private:
    inline static System::Random random_;
};

} // namespace Yacht
