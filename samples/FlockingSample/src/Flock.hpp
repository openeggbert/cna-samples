#pragma once
#include <memory>
#include <vector>
#include "AIParameters.hpp"
#include "Animals/Bird.hpp"
#include "Animals/Cat.hpp"
#include "System/Random.hpp"

namespace Flocking {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

// Birds are heap-allocated so Behavior raw pointers into them stay valid
// across vector growth (Bird contains self-referencing Behavior objects).
class Flock {
    static constexpr int flockSize = 40;

    int boundaryWidth_;
    int boundaryHeight_;

    Texture2D birdTexture_;
    std::vector<std::unique_ptr<Bird>> flock_;
    AIParameters flockParams_;

public:
    Flock(Texture2D tex, int screenWidth, int screenHeight,
          const AIParameters& params)
        : boundaryWidth_(screenWidth)
        , boundaryHeight_(screenHeight)
        , birdTexture_(std::move(tex))
        , flockParams_(params)
    {
        ResetFlock();
    }

    void SetFlockParams(const AIParameters& p) { flockParams_ = p; }

    void ResetFlock() {
        flock_.clear();
        System::Random random;
        for (int i = 0; i < flockSize; i++) {
            Vector2 loc((float)random.Next(boundaryWidth_),
                        (float)random.Next(boundaryHeight_));
            Vector2 dir((float)random.NextDouble() - 0.5f,
                        (float)random.NextDouble() - 0.5f);
            dir.Normalize();
            flock_.push_back(std::make_unique<Bird>(
                birdTexture_, dir, loc, boundaryWidth_, boundaryHeight_));
        }
    }

    void Update(const GameTime& gameTime, Cat* cat) {
        for (auto& thisBird : flock_) {
            thisBird->ResetThink();
            for (auto& otherBird : flock_) {
                if (thisBird.get() != otherBird.get())
                    thisBird->ReactTo(otherBird.get(), flockParams_);
            }
            thisBird->ReactTo(cat, flockParams_);
            thisBird->Update(gameTime, flockParams_);
        }
    }

    void Draw(SpriteBatch& spriteBatch, const GameTime& gameTime) {
        for (auto& bird : flock_)
            bird->Draw(spriteBatch, gameTime);
    }
};

} // namespace Flocking
