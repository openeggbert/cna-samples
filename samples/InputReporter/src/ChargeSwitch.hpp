#pragma once
#include <functional>
#include <stdexcept>
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"

namespace InputReporter {

class ChargeSwitch {
public:
    std::function<void()> Fire;

    explicit ChargeSwitch(float duration) { Reset(duration); }
    virtual ~ChargeSwitch() = default;

    bool Active() const { return active_; }

    void Update(const Microsoft::Xna::Framework::GameTime& gameTime,
                const Microsoft::Xna::Framework::Input::GamePadState& gamePadState) {
        active_ = IsCharging(gamePadState);
        if (active_) {
            if (remaining_ > 0.0f) {
                remaining_ -= (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
                if (remaining_ <= 0.0f && Fire)
                    Fire();
            }
        } else {
            Reset(duration_);
        }
    }

    void Reset(float duration) {
        if (duration < 0.0f)
            throw std::out_of_range("duration");
        remaining_ = duration_ = duration;
    }

protected:
    virtual bool IsCharging(const Microsoft::Xna::Framework::Input::GamePadState&) const = 0;

private:
    float duration_  = 3.0f;
    float remaining_ = 0.0f;
    bool  active_    = false;
};

} // namespace InputReporter
