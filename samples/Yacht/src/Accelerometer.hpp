#pragma once

#include <cmath>
#include <functional>
#include <optional>

#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Devices/Sensors/Accelerometer.hpp"
#include "Microsoft/Devices/Sensors/AccelerometerReading.hpp"
#include "Microsoft/Devices/Sensors/AccelerometerFailedException.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;

// An encapsulation of the accelerometer's current state (mirrors the XNA
// original's AccelerometerState struct in Misc/Accelerometer.cs).
struct AccelerometerState {
    Vector3 Acceleration;
    bool IsActive = false;
};

// A static encapsulation of accelerometer input, ported from
// Misc/Accelerometer.cs.
//
// CNA's Microsoft::Devices::Sensors::Accelerometer is a real SDL_Sensor-
// backed implementation (unlike the XNA-Phone-only original), gated on
// getIsSupportedProperty() exactly like the original's own
// `Environment.DeviceType == DeviceType.Device` check:
//   - Supported (real hardware): Start(), then poll getCurrentValueProperty()
//     once per frame via GetState().
//   - Not supported (this desktop, the normal case): the original's own
//     keyboard arrow-key emulation fallback (Left/Right/Up/Down -> synthetic
//     tilt vector), ported verbatim.
// The original's own `lock (threadLock)` is dropped: CNA's polling and the
// keyboard fallback both run on the main thread every frame, so there is no
// cross-thread race left to guard against.
//
// One deliberate behavioural difference from the stock sample: in the
// shipped C# original, CheckForShake/ShakeDetected are wired up *only*
// inside the real-hardware ReadingChanged event handler -- Accelerometer.
// GetState() is never even called anywhere in Yacht's own code, so on the
// emulator/non-device path (the only path reachable on this desktop) shake
// detection is simply dead code in the original. The approved plan calls
// for a working keyboard-fallback shake gesture so the feature is
// verifiable here, so GetState() below runs the same CheckForShake
// algorithm every frame regardless of which path produced the value.
class Accelerometer {
public:
    inline static std::function<void()> ShakeDetected;

    // Initializes the Accelerometer for the current game. Safe to call once.
    static void Initialize() {
        if (isInitialized_)
            return;

        if (Microsoft::Devices::Sensors::Accelerometer::getIsSupportedProperty()) {
            try {
                sensor_.emplace();
                sensor_->Start();
                isActive_ = true;
            } catch (const Microsoft::Devices::Sensors::AccelerometerFailedException&) {
                isActive_ = false;
            }
        } else {
            // We always report active on non-device platforms because we use
            // the arrow keys for simulation, which is always available.
            isActive_ = true;
        }

        isInitialized_ = true;
    }

    // Gets the current state of the accelerometer. Must be polled once per
    // frame for shake detection to work (see class comment above).
    static AccelerometerState GetState() {
        Vector3 stateValue;

        if (isActive_) {
            if (sensor_.has_value()) {
                stateValue = sensor_->getCurrentValueProperty().getAccelerationProperty();
            } else {
                // Keyboard arrow-key emulation, ported verbatim from the
                // original's own emulator fallback.
                KeyboardState keyboardState = Keyboard::GetState();

                stateValue.Z = -1;

                if (keyboardState.IsKeyDown(Keys::Left))
                    stateValue.X = -.1f;
                if (keyboardState.IsKeyDown(Keys::Right))
                    stateValue.X = .1f;
                if (keyboardState.IsKeyDown(Keys::Up))
                    stateValue.Y = .1f;
                if (keyboardState.IsKeyDown(Keys::Down))
                    stateValue.Y = -.1f;

                stateValue.Normalize();
            }

            CheckForShake(stateValue);
            lastValue_ = stateValue;
        }

        return AccelerometerState{stateValue, isActive_};
    }

private:
    // Check if the phone (or its keyboard-emulated stand-in) is shaking.
    static void CheckForShake(const Vector3& currentValue) {
        double deltaX = std::abs((double)(lastValue_.X - currentValue.X));
        double deltaY = std::abs((double)(lastValue_.Y - currentValue.Y));
        double deltaZ = std::abs((double)(lastValue_.Z - currentValue.Z));

        bool overShakeThreshold = (deltaX > shakeThreshold_ && deltaY > shakeThreshold_) ||
                                  (deltaX > shakeThreshold_ && deltaZ > shakeThreshold_) ||
                                  (deltaY > shakeThreshold_ && deltaZ > shakeThreshold_);

        bool overSettleThreshold = (deltaX > settleThreshold_ && deltaY > settleThreshold_) ||
                                   (deltaX > settleThreshold_ && deltaZ > settleThreshold_) ||
                                   (deltaY > settleThreshold_ && deltaZ > settleThreshold_);

        if (!shaking_ && overShakeThreshold && shakeCount_ >= 4) {
            shaking_ = true;
            shakeCount_ = 0;
            if (ShakeDetected)
                ShakeDetected();
        } else if (overShakeThreshold) {
            shakeCount_++;
        } else if (!overSettleThreshold) {
            shakeCount_ = 0;
            shaking_ = false;
        }
    }

    inline static std::optional<Microsoft::Devices::Sensors::Accelerometer> sensor_;
    inline static Vector3 lastValue_;
    inline static bool isActive_ = false;
    inline static bool isInitialized_ = false;
    inline static bool shaking_ = false;
    inline static int shakeCount_ = 0;
    static constexpr double shakeThreshold_ = 0.3;
    static constexpr double settleThreshold_ = 0.1;
};

} // namespace Yacht
