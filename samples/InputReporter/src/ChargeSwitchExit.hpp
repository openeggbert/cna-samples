#pragma once
#include "ChargeSwitch.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"

namespace InputReporter {

class ChargeSwitchExit : public ChargeSwitch {
public:
    explicit ChargeSwitchExit(float duration) : ChargeSwitch(duration) {}

protected:
    bool IsCharging(const Microsoft::Xna::Framework::Input::GamePadState& state) const override {
        using Microsoft::Xna::Framework::Input::ButtonState;
        return state.getButtonsProperty().getBackProperty() == ButtonState::Pressed;
    }
};

} // namespace InputReporter
