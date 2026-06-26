#pragma once
#include <algorithm>
#include <vector>
#include "Move.hpp"
#include "InputManager.hpp"

namespace InputSequenceSample {

class MoveList {
    std::vector<Move> moves_;

public:
    explicit MoveList(std::vector<Move> moves) : moves_(std::move(moves)) {
        std::sort(moves_.begin(), moves_.end(),
            [](const Move& a, const Move& b) {
                return a.Sequence.size() > b.Sequence.size();
            });
    }

    Move* DetectMove(InputManager& input) {
        for (auto& move : moves_)
            if (input.Matches(move))
                return &move;
        return nullptr;
    }

    int LongestMoveLength() const {
        return moves_.empty() ? 0 : (int)moves_[0].Sequence.size();
    }
};

} // namespace InputSequenceSample
