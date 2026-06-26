#pragma once
#include "ChargeSwitch.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"

namespace InputReporter {

class ChargeSwitchDeadZone : public ChargeSwitch {
public:
    explicit ChargeSwitchDeadZone(float duration) : ChargeSwitch(duration) {}

protected:
    bool IsCharging(const Microsoft::Xna::Framework::Input::GamePadState& state) const override {
        using Microsoft::Xna::Framework::Input::ButtonState;
        return state.getButtonsProperty().getStartProperty() == ButtonState::Pressed;
    }
};

} // namespace InputReporter
