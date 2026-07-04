#pragma once

// ScoreBar.hpp — C++ port of Objects/ScoreBar.cs (XNA 4.0 HoneycombRush
// sample). Displays a fill-bar UI widget (used for honey jars, beehives, the
// vat, and the smoke gauge), colored red/yellow/green by fill percentage.

#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace HoneycombRush {

class GameplayScreen; // forward declaration

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Used by other components to display their status. Port of Objects/ScoreBar.cs.
class ScoreBar : public DrawableGameComponent {
public:
    enum class ScoreBarOrientation { Vertical, Horizontal };

    int MinValue;
    int MaxValue;
    Vector2 Position;
    Color BarColor;

    int CurrentValue() const { return currentValue_; }

    ScoreBar(Game& game, int minValue, int maxValue, Vector2 position, int height, int width, Color scoreBarColor,
             ScoreBarOrientation orientation, int initialValue, GameplayScreen* screen, bool isAppearAtCountDown)
        : DrawableGameComponent(game),
          MinValue(minValue),
          MaxValue(maxValue),
          Position(position),
          BarColor(scoreBarColor),
          orientation_(orientation),
          height_(height),
          width_(width),
          currentValue_(initialValue),
          gameplayScreen_(screen),
          isAppearAtCountDown_(isAppearAtCountDown) {
        spriteBatch_ = game.getServicesProperty().GetService<SpriteBatch>();
    }

    void LoadContent() override {
        auto& content = getGameProperty().getContentProperty();
        backgroundTexture_.emplace(content.Load<Texture2D>("Textures/barBlackBorder"));
        greenTexture_.emplace(content.Load<Texture2D>("Textures/barGreen"));
        yellowTexture_.emplace(content.Load<Texture2D>("Textures/barYellow"));
        redTexture_.emplace(content.Load<Texture2D>("Textures/barRed"));
    }

    // Defined out-of-line in GameplayScreen.hpp, since it needs
    // GameplayScreen::IsActive()/IsStarted() (an incomplete type here).
    void Draw(const GameTime& gameTime) override;

    void IncreaseCurrentValue(int valueToIncrease) {
        if (valueToIncrease >= 0 && currentValue_ < MaxValue && currentValue_ + valueToIncrease <= MaxValue) {
            currentValue_ += valueToIncrease;
        } else if (currentValue_ + valueToIncrease > MaxValue) {
            currentValue_ = MaxValue;
        }
    }

    void DecreaseCurrentValue(int valueToDecrease) { DecreaseCurrentValue(valueToDecrease, false); }

    int DecreaseCurrentValue(int valueToDecrease, bool clampToMinimum) {
        int valueThatWasDecreased = 0;
        if (valueToDecrease >= 0 && currentValue_ > MinValue && currentValue_ - valueToDecrease >= MinValue) {
            currentValue_ -= valueToDecrease;
            valueThatWasDecreased = valueToDecrease;
        } else if (currentValue_ - valueToDecrease < MinValue && clampToMinimum) {
            valueThatWasDecreased = currentValue_ - MinValue;
            currentValue_ = MinValue;
        }
        return valueThatWasDecreased;
    }

    ScoreBarOrientation Orientation() const { return orientation_; }
    int Height() const { return height_; }
    int Width() const { return width_; }
    bool IsAppearAtCountDown() const { return isAppearAtCountDown_; }
    SpriteBatch* GetSpriteBatch() const { return spriteBatch_; }
    Texture2D& GetBackgroundTexture() { return *backgroundTexture_; }

    // Calculates the empty portion of the bar according to its current value.
    float GetSpaceFromBorder() const {
        float textureSize = (float)width_;
        float valuePercent = MaxValue != 0 ? ((float)currentValue_ / (float)MaxValue) * 100.0f : 0.0f;
        return textureSize - (textureSize * valuePercent / 100.0f);
    }

    // Returns a texture for the bar's "fill", colored according to its value.
    Texture2D& GetTextureByCurrentValue() {
        float valuePercent = MaxValue != 0 ? ((float)currentValue_ / (float)MaxValue) * 100.0f : 0.0f;
        if (valuePercent > 50.0f) return *greenTexture_;
        if (valuePercent > 25.0f) return *yellowTexture_;
        return *redTexture_;
    }

private:
    ScoreBarOrientation orientation_;
    int height_;
    int width_;

    SpriteBatch* spriteBatch_ = nullptr;
    int currentValue_;

    std::optional<Texture2D> backgroundTexture_;
    std::optional<Texture2D> redTexture_;
    std::optional<Texture2D> greenTexture_;
    std::optional<Texture2D> yellowTexture_;

    GameplayScreen* gameplayScreen_ = nullptr;
    bool isAppearAtCountDown_ = false;
};

} // namespace HoneycombRush
