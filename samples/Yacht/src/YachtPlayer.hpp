#pragma once

#include <string>
#include <utility>

#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Graphics::SpriteBatch;

class DiceHandler;       // non-owning back-pointer (shared dice)
class GameStateHandler;  // non-owning back-pointer (cycle-break, resolved
                         // out-of-line in GameStateHandler.hpp for
                         // HumanPlayer/AIPlayer -- the same idiom used by
                         // CatapultWars' Catapult<->Player cycle, see
                         // NEXT.md).

// Abstract base for the two kinds of Yacht players (no separate Player.cs
// exists in the original XNA sample -- YachtPlayer.cs is itself already
// abstract). NetworkPlayer is dropped per the approved plan (online play is
// out of scope for this port).
class YachtPlayer {
public:
    YachtPlayer(std::string name, DiceHandler* diceHandler)
        : name_(std::move(name)), diceHandler_(diceHandler) {}
    virtual ~YachtPlayer() = default;

    const std::string& getName() const { return name_; }
    void setName(const std::string& value) { name_ = value; }

    DiceHandler* getDiceHandler() const { return diceHandler_; }

    GameStateHandler* getGameStateHandler() const { return gameStateHandler_; }
    void setGameStateHandler(GameStateHandler* value) { gameStateHandler_ = value; }

    // Allows the player to perform a portion of its playing logic.
    virtual void PerformPlayerLogic() {}

    // Draws information related to the player.
    virtual void Draw(SpriteBatch& spriteBatch) { (void)spriteBatch; }

protected:
    std::string name_;
    DiceHandler* diceHandler_ = nullptr;
    GameStateHandler* gameStateHandler_ = nullptr;
};

} // namespace Yacht
