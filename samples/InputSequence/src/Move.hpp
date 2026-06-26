#pragma once
#include <string>
#include <vector>
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"

namespace InputSequenceSample {

struct Move {
    std::string Name;
    std::vector<Microsoft::Xna::Framework::Input::Buttons> Sequence;
    bool IsSubMove = false;

    Move(std::string name,
         std::vector<Microsoft::Xna::Framework::Input::Buttons> sequence,
         bool isSubMove = false)
        : Name(std::move(name))
        , Sequence(std::move(sequence))
        , IsSubMove(isSubMove)
    {}
};

} // namespace InputSequenceSample
