#pragma once

#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "System/InvalidOperationException.hpp"

namespace AccelerometerSample {

// Port of the XNA 4.0 original's static `Accelerometer` class
// (Accelerometer.cs). The real Windows Phone hardware path -- the
// `#if WINDOWS_PHONE` block wrapping `Microsoft.Devices.Sensors.Accelerometer`
// -- has no equivalent on desktop (no phone SDK, no phone hardware) and is
// not ported.
//
// What IS ported, verbatim, is the original's own *emulator* fallback
// (Accelerometer.cs:117-135, inside the same `#if WINDOWS_PHONE` block,
// gated on `Microsoft.Devices.Environment.DeviceType != DeviceType.Device`):
// when the sample runs in the Visual Studio Windows Phone 7 *emulator*
// instead of on a real device, GetState() doesn't read real hardware at
// all -- it synthesizes a Vector3 from the arrow keys instead:
//
//     stateValue.Z = -1;
//     if (keyboardState.IsKeyDown(Keys.Left))  stateValue.X--;
//     if (keyboardState.IsKeyDown(Keys.Right)) stateValue.X++;
//     if (keyboardState.IsKeyDown(Keys.Up))    stateValue.Y++;
//     if (keyboardState.IsKeyDown(Keys.Down))  stateValue.Y--;
//     stateValue.Normalize();
//
// This is the ONLY implementation in this port -- there is no #ifdef/
// real-sensor branch, because this desktop build has no phone-style
// accelerometer emulator distinction to make: it always takes the same
// path the original's own emulator build took. This is not an invented
// control scheme -- it's the original sample's own already-designed
// fallback, promoted from conditional (`#if WINDOWS_PHONE` + a DeviceType
// runtime check) to unconditional, the same "un-#if an existing branch"
// pattern already established by Yacht/SnowShovel/Bounce (see
// DEFERRED.md item #15). Unlike those three samples, though, this one has
// no *separate* non-phone input branch to promote -- the emulator branch
// itself, above, is the only non-hardware code path the original ever
// shipped, so it's what gets promoted here.
class Accelerometer {
public:
    struct State {
        Microsoft::Xna::Framework::Vector3 Acceleration;
        bool IsActive = false;
    };

    // Matches Accelerometer.Initialize(): can only be called once per game.
    void Initialize() {
        if (initialized_) {
            throw System::InvalidOperationException("Initialize can only be called once");
        }

        // The original's emulator branch always sets isActive = true
        // ("we always return isActive on emulator because we use the
        // arrow keys for simulation which is always available").
        isActive_ = true;
        initialized_ = true;
    }

    // Matches Accelerometer.GetState().
    State GetState() const {
        if (!initialized_) {
            throw System::InvalidOperationException("You must Initialize before you can call GetState");
        }

        Microsoft::Xna::Framework::Vector3 stateValue;

        if (isActive_) {
            using Microsoft::Xna::Framework::Input::Keyboard;
            using Microsoft::Xna::Framework::Input::Keys;

            auto keyboardState = Keyboard::GetState();

            stateValue.Z = -1;

            if (keyboardState.IsKeyDown(Keys::Left))
                stateValue.X -= 1;
            if (keyboardState.IsKeyDown(Keys::Right))
                stateValue.X += 1;
            if (keyboardState.IsKeyDown(Keys::Up))
                stateValue.Y += 1;
            if (keyboardState.IsKeyDown(Keys::Down))
                stateValue.Y -= 1;

            stateValue.Normalize();
        }

        return State{stateValue, isActive_};
    }

private:
    bool initialized_ = false;
    bool isActive_ = false;
};

} // namespace AccelerometerSample
