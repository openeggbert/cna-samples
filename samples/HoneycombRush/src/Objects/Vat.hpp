#pragma once

// Vat.hpp — C++ port of Objects/Vat.cs (XNA 4.0 HoneycombRush sample).
// Draw() is declared here but defined out-of-line in GameplayScreen.hpp —
// see missing.md.

#include <cstdio>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/TimeSpan.hpp"

#include "ScoreBar.hpp"
#include "TexturedDrawableGameComponent.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Graphics::SpriteFont;

// A game component that represents the vat. Port of Objects/Vat.cs.
class Vat : public TexturedDrawableGameComponent {
public:
    static constexpr const char* EmptyString = "Empty";
    static constexpr const char* FullString = "Full";
    static constexpr const char* TimeLeftString = "Time Left";

    Vat(Game& game, GameplayScreen* gamePlayScreen, Texture2D tex, Vector2 pos, ScoreBar* score)
        : TexturedDrawableGameComponent(game, gamePlayScreen), score_(score) {
        texture = std::move(tex);
        position = pos;
        setDrawOrderProperty((int)(pos.Y + Bounds().Height));
    }

    void LoadContent() override {
        auto& content = getGameProperty().getContentProperty();
        font14px_.emplace(content.Load<SpriteFont>("Fonts/GameScreenFont14px"));
        font16px_.emplace(content.Load<SpriteFont>("Fonts/GameScreenFont16px"));
        font36px_.emplace(content.Load<SpriteFont>("Fonts/GameScreenFont36px"));

        fullStringSize_ = font14px_->MeasureString(FullString);
        emptyStringSize_ = font14px_->MeasureString(EmptyString);
        timeleftStringSize_ = font16px_->MeasureString(TimeLeftString);

        TexturedDrawableGameComponent::LoadContent();
    }

    void Draw(const GameTime& gameTime) override;

    Rectangle CentralCollisionArea() const override {
        Rectangle bounds = Bounds();
        int height = bounds.Height / 10 * 5;
        int width = bounds.Width / 10 * 8;
        int offsetY = (bounds.Height - height) / 2;
        int offsetX = (bounds.Width - width) / 2;
        return Rectangle(bounds.X + offsetX, bounds.Y + offsetY, width, height);
    }

    Rectangle VatDepositArea() const {
        Rectangle bounds = Bounds();
        float sizeFactor = 0.75f;
        float marginFactor = (1.0f - sizeFactor) / 2.0f;
        int x = bounds.X + (int)(marginFactor * (float)bounds.Width);
        int y = bounds.Y + (int)(marginFactor * (float)bounds.Height);
        int width = (int)((float)bounds.Width * sizeFactor);
        int height = (int)((float)bounds.Height * sizeFactor);
        return Rectangle(x, y, width, height);
    }

    int MaxVatCapacity() const { return score_->MaxValue; }
    int CurrentVatCapacity() const { return score_->CurrentValue(); }

    void DrawTimeLeft(System::TimeSpan timeLeft) {
        timeLeft_ = timeLeft;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d", timeLeft.getMinutesProperty(), timeLeft.getSecondsProperty());
        timeLeftString_ = buf;
    }

    void IncreaseHoney(int value) { score_->IncreaseCurrentValue(value); }

    SpriteFont& Font14px() { return *font14px_; }
    SpriteFont& Font16px() { return *font16px_; }
    SpriteFont& Font36px() { return *font36px_; }
    Vector2 EmptyStringSize() const { return emptyStringSize_; }
    Vector2 FullStringSize() const { return fullStringSize_; }
    Vector2 TimeLeftStringSize() const { return timeleftStringSize_; }
    const std::string& TimeLeftText() const { return timeLeftString_; }
    System::TimeSpan TimeLeft() const { return timeLeft_; }

private:
    ScoreBar* score_;
    std::optional<SpriteFont> font14px_, font16px_, font36px_;
    Vector2 emptyStringSize_, fullStringSize_, timeleftStringSize_;
    System::TimeSpan timeLeft_;
    std::string timeLeftString_;
};

} // namespace HoneycombRush
