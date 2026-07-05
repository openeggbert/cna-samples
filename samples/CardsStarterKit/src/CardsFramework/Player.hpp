#pragma once

// Player.hpp -- C++ port of Players/Player.cs (XNA 4.0 CardsStarterKit sample).
// Base player to be extended by inheritance for each card game.

#include <string>

#include "Hand.hpp"

namespace CardsFramework {

class CardsGame;  // forward declaration

class Player {
public:
    std::string Name;
    CardsGame* Game;
    Hand PlayerHand;

    Player(const std::string& name, CardsGame* game) : Name(name), Game(game) {}
    virtual ~Player() = default;
};

} // namespace CardsFramework
