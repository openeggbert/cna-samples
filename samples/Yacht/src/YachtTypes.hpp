#pragma once

#include <array>
#include <string>
#include <vector>

namespace Yacht {

// Sentinel meaning "no score set yet" for a score-card entry (ported from
// YachtServices/ServiceConstants.cs). Serialization and networking are
// dropped, so this is the only piece of that file this port needs.
constexpr int NullScore = 255;

// Possible score types on the score card (subset of
// Objects/GameStateHandler.cs's YachtCombination enum -- unchanged, it was
// never network-specific).
enum class YachtCombination {
    Ones = 1,
    Twos = 2,
    Threes = 3,
    Fours = 4,
    Fives = 5,
    Sixes = 6,
    Choise = 7,  // [sic] matches the original's own spelling
    FullHouse = 8,
    FourOfAKind = 9,
    SmallStraight = 10,
    LargeStraight = 11,
    Yacht = 12,
};

// Plain-data subset of YachtServices/DataModel.cs's PlayerInformation, with
// serialization and the online-only PlayerID/AIPlayerBehavior/Timer fields
// dropped (online play is out of scope for this port).
struct PlayerInformation {
    std::string Name;
    std::array<int, 12> ScoreCard{};  // filled with NullScore by GameStateHandler
    int TotalScore = 0;
};

// Plain-data subset of YachtServices/DataModel.cs's GameState. The
// GameID/Name/GameType/IsStarted/TimerToDelete/SequenceNumber fields only
// mattered for online play and XML serialization, and are dropped -- this
// port is always a fresh, local, offline game.
struct GameState {
    std::vector<PlayerInformation> Players;
    int StepsMade = 0;
    int CurrentPlayer = 0;
};

} // namespace Yacht
