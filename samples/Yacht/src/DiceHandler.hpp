#pragma once

#include <array>
#include <memory>
#include <optional>

#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "Dice.hpp"
#include "AudioManager.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Graphics::GraphicsDevice;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;

// Manages and draws a player's five dice. Merges Objects/DiceHandler.cs and
// Objects/DiceState.cs -- DiceState only ever existed to support
// tombstoning (save/load) serialization, which this port drops entirely, so
// its two arrays (RollingDice[5]/HoldingDice[5]) are folded directly into
// this class.
class DiceHandler {
public:
    static constexpr int DiceAmount = 5;

    explicit DiceHandler(GraphicsDevice& graphicsDevice) {
        screenBounds_ = graphicsDevice.getViewportProperty().getBoundsProperty();
        Reset(false);
    }

    // Loads assets used by the dice handler and performs other visual
    // initializations. `font` is used to draw the "HOLD" label (the
    // original reaches for the YachtGame.Font static from anywhere; this
    // port threads the font in explicitly to avoid a header-only circular
    // include between YachtGame.hpp and the gameplay object headers it
    // composes -- see missing.md).
    void LoadAssets(ContentManager& content, SpriteFont& font) {
        font_ = &font;
        diceRollBorder_.emplace(content.Load<Texture2D>("Images/diceRollBorder"));
        holdingTray_.emplace(content.Load<Texture2D>("Images/holdingTray"));

        holdingTrayPosition_ = Vector2((float)screenBounds_.getLeftProperty(),
                                       (float)(screenBounds_.getBottomProperty() - holdingTray_->getHeightProperty()));
        holdTextPosition_ = holdingTrayPosition_ + Vector2(20, 30);
        rollBorderPosition_ = holdingTrayPosition_ - Vector2(0, (float)diceRollBorder_->getHeightProperty() + 10);

        PositionDice();
    }

    int getRolls() const { return rolls_; }

    void Update(GameTime& gameTime) {
        for (auto& d : rollingDice_)
            if (d) d->Update(gameTime);

        // Place all dice on the holding tray after 3 rolls.
        if (rolls_ == 3 && !DiceRolling()) {
            for (int i = 0; i < DiceAmount; i++)
                if (rollingDice_[i])
                    MoveDice(i);
        }

        if (DiceRolling())
            AudioManager::PlaySoundRandom("Roll", 4);
    }

    void Draw(SpriteBatch& spriteBatch) {
        spriteBatch.Draw(*holdingTray_, holdingTrayPosition_, Color::White);
        spriteBatch.DrawString(*font_, "HOLD", holdTextPosition_, Color::White);
        spriteBatch.Draw(*diceRollBorder_, rollBorderPosition_, Color::White);
        DrawDice(spriteBatch);
    }

    // Returns the 5 held dice (nullptr for empty slots), or nullopt if none
    // are currently held.
    std::optional<std::array<Dice*, DiceAmount>> getHoldingDice() const {
        return SnapshotIfAny(holdingDice_);
    }

    // Returns the 5 rolling dice (nullptr for empty slots), or nullopt if
    // none are currently in the rolling border.
    std::optional<std::array<Dice*, DiceAmount>> getRollingDice() const {
        return SnapshotIfAny(rollingDice_);
    }

    // Resets the handler before starting a new turn.
    void Reset(bool gameOver) {
        Reset(gameOver, true);
    }

    void Reset(bool gameOver, bool resetRolls) {
        if (resetRolls)
            rolls_ = 0;

        for (int i = 0; i < DiceAmount; i++) {
            if (gameOver) {
                rollingDice_[i].reset();
            } else {
                rollingDice_[i] = std::make_unique<Dice>();
                rollingDice_[i]->Position =
                    Vector2(rollBorderPosition_.X + 89.0f * (float)i + 30.0f, rollBorderPosition_.Y + 20.0f);
            }
            holdingDice_[i].reset();
        }
    }

    // Put dice in the right positions on the screen.
    void PositionDice() {
        for (int i = 0; i < DiceAmount; i++) {
            if (rollingDice_[i]) {
                rollingDice_[i]->Position =
                    Vector2(rollBorderPosition_.X + 89.0f * (float)i + 30.0f, rollBorderPosition_.Y + 20.0f);
            } else if (holdingDice_[i]) {
                holdingDice_[i]->Position =
                    Vector2(rollBorderPosition_.X + 89.0f * (float)i + 30.0f,
                            rollBorderPosition_.Y + 30.0f + (float)holdingTray_->getHeightProperty());
            }
        }
    }

    // Roll the dice inside the rolling border.
    void Roll() {
        if (rolls_ < 3 && !DiceRolling()) {
            for (int i = 0; i < DiceAmount; i++)
                if (rollingDice_[i])
                    rollingDice_[i]->Roll();

            rolls_++;
        }
    }

    // Move a die from the roll border to the hold tray or the other way around.
    void MoveDice(int diceIndex) {
        if (rolls_ > 0 && diceIndex >= 0 && diceIndex < DiceAmount) {
            if (rollingDice_[diceIndex] && !rollingDice_[diceIndex]->IsRolling()) {
                rollingDice_[diceIndex]->Position =
                    rollingDice_[diceIndex]->Position + Vector2(0, (float)holdingTray_->getHeightProperty() + 10);
                holdingDice_[diceIndex] = std::move(rollingDice_[diceIndex]);
                rollingDice_[diceIndex].reset();
                AudioManager::PlaySoundRandom("DieSelect", 2);
            } else if (holdingDice_[diceIndex]) {
                holdingDice_[diceIndex]->Position =
                    holdingDice_[diceIndex]->Position - Vector2(0, (float)holdingTray_->getHeightProperty() + 10);
                rollingDice_[diceIndex] = std::move(holdingDice_[diceIndex]);
                holdingDice_[diceIndex].reset();
                AudioManager::PlaySoundRandom("DieSelect", 2);
            }
        }
    }

    bool DiceRolling() const {
        for (const auto& d : rollingDice_)
            if (d && d->IsRolling())
                return true;
        return false;
    }

private:
    void DrawDice(SpriteBatch& spriteBatch) {
        for (int i = 0; i < DiceAmount; i++) {
            if (rollingDice_[i])
                rollingDice_[i]->Draw(spriteBatch);
            else if (holdingDice_[i])
                holdingDice_[i]->Draw(spriteBatch);
        }
    }

    static std::optional<std::array<Dice*, DiceAmount>> SnapshotIfAny(
        const std::array<std::unique_ptr<Dice>, DiceAmount>& dice) {
        bool any = false;
        std::array<Dice*, DiceAmount> out{};
        for (int i = 0; i < DiceAmount; i++) {
            out[i] = dice[i].get();
            if (out[i] != nullptr) any = true;
        }
        if (!any) return std::nullopt;
        return out;
    }

    std::array<std::unique_ptr<Dice>, DiceAmount> rollingDice_;
    std::array<std::unique_ptr<Dice>, DiceAmount> holdingDice_;
    int rolls_ = 0;

    SpriteFont* font_ = nullptr;
    std::optional<Texture2D> diceRollBorder_;
    std::optional<Texture2D> holdingTray_;

    Vector2 holdingTrayPosition_;
    Vector2 holdTextPosition_;
    Vector2 rollBorderPosition_;

    Rectangle screenBounds_;
};

} // namespace Yacht
