#pragma once

#include <array>
#include <functional>
#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "System/Random.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// Possible die values.
enum class DiceValue {
    One = 1,
    Two = 2,
    Three = 3,
    Four = 4,
    Five = 5,
    Six = 6,
};

// A six-sided die which can draw itself on screen. Ported from
// Objects/Dice.cs.
//
// The original used a shared `static Random` plus a self-rearming
// System.Threading.Timer to animate the roll. Per the approved plan, the
// Timer is replaced with a per-die elapsed-time accumulator driven from
// Update(GameTime&) (called every frame from DiceHandler::Update); random_
// stays an inline static System::Random member shared by every die --
// matching both the original's one-shared-RNG-across-all-dice behaviour and
// this repo's existing AudioManager.hpp idiom for shared statics.
class Dice {
public:
    Dice() = default;

    // Loads assets that will be used by all instances.
    static void LoadAssets(ContentManager& content) {
        diceStrip_.emplace(content.Load<Texture2D>("Images/dice"));

        int faceWidth = diceStrip_->getWidthProperty() / (int)diceFaces_.size();
        for (size_t i = 0; i < diceFaces_.size(); i++) {
            diceFaces_[i] = Rectangle((int)i * faceWidth, 0, faceWidth, diceStrip_->getHeightProperty());
        }
    }

    // Event launched when the die is tapped. Kept for parity with the
    // original even though the stock sample never actually wires this up --
    // dice hit-testing is done directly by HumanPlayer::TryMoveDiceAt.
    std::function<void()> Click;

    DiceValue Value = DiceValue::One;
    Vector2 Position;

    bool IsRolling() const { return isRolling_; }

    // Update the die: advances the roll animation timer.
    void Update(GameTime& gameTime) {
        if (!isRolling_)
            return;

        if (!timerArmed_) {
            rollTimer_ = random_.Next(300, 600) / 1000.0f;
            timerArmed_ = true;
            return;
        }

        rollTimer_ -= (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        if (rollTimer_ <= 0.0f) {
            timerArmed_ = false;
            Value = (DiceValue)random_.Next(1, 7);
            isRolling_ = random_.Next(0, 5) != 1;
        }
    }

    void HandleInput(const GestureSample& sample) {
        if (sample.getGestureTypeProperty() == GestureType::Tap) {
            Rectangle touchRect((int)sample.getPositionProperty().X - 1,
                                (int)sample.getPositionProperty().Y - 1, 2, 2);
            if (Intersects(touchRect) && Click)
                Click();
        }
    }

    void Draw(SpriteBatch& spriteBatch) {
        spriteBatch.Draw(*diceStrip_, FaceBounds(), diceFaces_[(int)Value - 1], Color::White);
    }

    void Roll() {
        isRolling_ = true;
    }

    bool Intersects(Rectangle rect) const {
        return FaceBounds().Intersects(rect);
    }

private:
    Rectangle FaceBounds() const {
        Rectangle bounds = diceFaces_[0];
        bounds.X += (int)Position.X;
        bounds.Y += (int)Position.Y;
        return bounds;
    }

    bool isRolling_ = false;
    bool timerArmed_ = false;
    float rollTimer_ = 0.0f;

    inline static System::Random random_;
    inline static std::optional<Texture2D> diceStrip_;
    inline static std::array<Rectangle, 6> diceFaces_{};
};

// Ordering used when scoring dice combinations (mirrors Objects/Dice.cs's
// IComparable<Dice> implementation). .NET's Array.Sort(Dice[]) uses the
// generic default comparer, which treats a null reference as always "less
// than" any non-null instance (it never calls CompareTo when either operand
// is null -- Dice.CompareTo's own `other != null` guard is for a different,
// unreachable-via-Array.Sort call shape); non-null dice then compare by
// their numeric face value. `a`/`b` may be nullptr.
inline bool DiceLessThan(const Dice* a, const Dice* b) {
    if (a == nullptr) return b != nullptr;
    if (b == nullptr) return false;
    return (int)a->Value < (int)b->Value;
}

} // namespace Yacht
