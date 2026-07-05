#pragma once

// AnimatedGameComponent.hpp -- C++ port of UI/AnimatedGameComponent.cs (XNA 4.0
// CardsStarterKit sample). A game component that can display a variable
// texture/frame while managing a set of AnimatedGameComponentAnimations.

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "AnimatedGameComponentAnimation.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class CardsGame;  // forward declaration (Game.hpp)

// A game component which can display a variable frame while managing a set
// of running animations.
class AnimatedGameComponent : public DrawableGameComponent {
public:
    Texture2D* CurrentFrame = nullptr;
    std::optional<Rectangle> CurrentSegment;
    std::optional<std::string> Text;
    Color TextColor = Color::Black;
    bool IsFaceDown = true;
    Vector2 CurrentPosition;
    std::optional<Rectangle> CurrentDestination;

    // Whether or not an animation belonging to the component is running.
    virtual bool IsAnimating() const { return !runningAnimations_.empty(); }

    CardsGame* CardGame = nullptr;

    explicit AnimatedGameComponent(Game& game) : DrawableGameComponent(game) {}

    AnimatedGameComponent(Game& game, Texture2D* currentFrame)
        : DrawableGameComponent(game), CurrentFrame(currentFrame) {}

    // The CardsGame overload additionally records the owning card game; its
    // Game& is obtained from the card game (defined out-of-line in
    // CardsGame.hpp, which is the only header that knows CardsGame's layout).
    AnimatedGameComponent(CardsGame& cardGame, Texture2D* currentFrame);

    // Keeps track of the component's animations.
    void Update(GameTime& gameTime) override {
        DrawableGameComponent::Update(gameTime);

        for (size_t i = 0; i < runningAnimations_.size();) {
            runningAnimations_[i]->AccumulateElapsedTime(gameTime.getElapsedGameTimeProperty());
            runningAnimations_[i]->Run(gameTime);
            if (runningAnimations_[i]->IsDone()) {
                runningAnimations_.erase(runningAnimations_.begin() + i);
            } else {
                i++;
            }
        }
    }

    // Draws the animated component and its text (if any) at its destination,
    // or its initial position if a destination is not set. Needs CardGame's
    // SpriteBatch/Font, so is defined out-of-line in CardsGame.hpp.
    void Draw(const GameTime& gameTime) override;

    // Adds an animation to the animated component.
    void AddAnimation(std::unique_ptr<AnimatedGameComponentAnimation> animation) {
        animation->Component = this;
        runningAnimations_.push_back(std::move(animation));
    }

    // Estimated time at which the longest-lasting managed animation completes.
    virtual TimeSpan EstimatedTimeForAnimationsCompletion() const {
        TimeSpan result = TimeSpan::Zero;
        if (IsAnimating()) {
            for (auto& anim : runningAnimations_) {
                if (anim->EstimatedTimeForAnimationCompletion() > result)
                    result = anim->EstimatedTimeForAnimationCompletion();
            }
        }
        return result;
    }

protected:
    std::vector<std::unique_ptr<AnimatedGameComponentAnimation>> runningAnimations_;
};

} // namespace CardsFramework
