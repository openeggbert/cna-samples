#pragma once

// HoneyJar.hpp — C++ port of Objects/HoneyJar.cs (XNA 4.0 HoneycombRush
// sample). Draw() is declared here but defined out-of-line in
// GameplayScreen.hpp — see missing.md.

#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "ScoreBar.hpp"
#include "TexturedDrawableGameComponent.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Graphics::SpriteFont;

// A game component that represents the honey jar. Port of Objects/HoneyJar.cs.
class HoneyJar : public TexturedDrawableGameComponent {
public:
    static constexpr const char* HoneyText = "Honey";

    HoneyJar(Game& game, GameplayScreen* gamePlayScreen, Vector2 pos, ScoreBar* score)
        : TexturedDrawableGameComponent(game, gamePlayScreen), score_(score) {
        position = pos;
    }

    void LoadContent() override {
        font16px_.emplace(getGameProperty().getContentProperty().Load<SpriteFont>("Fonts/GameScreenFont16px"));
        texture = getGameProperty().getContentProperty().Load<Texture2D>("Textures/honeyJar");
        honeyTextSize_ = font16px_->MeasureString(HoneyText);

        TexturedDrawableGameComponent::LoadContent();
    }

    void Draw(const GameTime& gameTime) override;

    bool CanCarryMore() const { return score_->CurrentValue() < score_->MaxValue; }
    bool HasHoney() const { return score_->CurrentValue() > score_->MinValue; }

    void IncreaseHoney(int value) { score_->IncreaseCurrentValue(value); }
    void DecreaseHoney(int value) { score_->DecreaseCurrentValue(value); }

    int DecreaseHoneyByPercent(int percent) {
        return score_->DecreaseCurrentValue(percent * score_->MaxValue / 100, true);
    }

    SpriteFont& Font16px() { return *font16px_; }
    Vector2 HoneyTextSize() const { return honeyTextSize_; }

private:
    ScoreBar* score_;
    std::optional<SpriteFont> font16px_;
    Vector2 honeyTextSize_;
};

} // namespace HoneycombRush
