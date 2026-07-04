#pragma once

// Beehive.hpp — C++ port of Objects/Beehive.cs (XNA 4.0 HoneycombRush
// sample). Update()/Draw() are declared here but defined out-of-line in
// GameplayScreen.hpp, since they need GameplayScreen::IsActive() (an
// incomplete type in this header) — see missing.md.

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/TimeSpan.hpp"

#include "ScoreBar.hpp"
#include "TexturedDrawableGameComponent.hpp"

namespace HoneycombRush {

// Represents a single beehive. Port of Objects/Beehive.cs.
class Beehive : public TexturedDrawableGameComponent {
public:
    bool AllowBeesToGenerate = true;

    Beehive(Game& game, GameplayScreen* gamePlayScreen, Texture2D tex, ScoreBar* score, Vector2 pos)
        : TexturedDrawableGameComponent(game, gamePlayScreen), score_(score) {
        texture = std::move(tex);
        position = pos;
        setDrawOrderProperty((int)pos.Y);
    }

    bool HasHoney() const { return score_->CurrentValue() > score_->MinValue; }

    Rectangle Bounds() const override {
        Rectangle base_ = TexturedDrawableGameComponent::Bounds();
        int width = base_.Width;
        int height = base_.Height / 3;
        return Rectangle(base_.X + 4, base_.Y + height + 10, width - 15, height);
    }

    Rectangle CentralCollisionArea() const override {
        Rectangle bounds = Bounds();
        int height = bounds.Height / 10 * 5;
        int width = bounds.Width / 10 * 4;
        int offsetY = (bounds.Height - height) / 2;
        int offsetX = (bounds.Width - width) / 2;
        return Rectangle(bounds.X + offsetX, bounds.Y + offsetY, width, height);
    }

    void Update(GameTime& gameTime) override;
    void Draw(const GameTime& gameTime) override;

    void DecreaseHoney(int amount) { score_->DecreaseCurrentValue(amount); }

private:
    ScoreBar* score_;
    System::TimeSpan intervalToAddHoney_ = System::TimeSpan::FromMilliseconds(600);
    System::TimeSpan lastTimeHoneyAdded_;

public:
    // Accessors used by the out-of-line Update()/Draw() in GameplayScreen.hpp.
    ScoreBar& ScoreBarRef() { return *score_; }
    System::TimeSpan& LastTimeHoneyAdded() { return lastTimeHoneyAdded_; }
    System::TimeSpan IntervalToAddHoney() const { return intervalToAddHoney_; }
};

} // namespace HoneycombRush
