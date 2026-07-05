#pragma once

// ScoreComponent.hpp — C++ port of Elements/HUD/ScoreComponent.cs (XNA 4.0
// NinjAcademy sample). A component used to display and maintain the game
// score.

#include "../../GameConstants.hpp"
#include "TextDisplayComponent.hpp"

namespace NinjAcademy {

// Port of Elements/HUD/ScoreComponent.cs.
class ScoreComponent : public TextDisplayComponent {
public:
    ScoreComponent(Game& game, SpriteFont& font) : TextDisplayComponent(game, font) {
        Position = GameConstants::ScorePosition;
        setScore(0);
    }

    int getScore() const { return score_; }
    void setScore(int value) {
        score_ = value;
        Text = "Score: " + std::to_string(score_);
    }

private:
    int score_ = 0;
};

} // namespace NinjAcademy
