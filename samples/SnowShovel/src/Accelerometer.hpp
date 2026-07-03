#pragma once

#include <optional>

#include "Microsoft/Devices/Sensors/Accelerometer.hpp"
#include "Microsoft/Devices/Sensors/AccelerometerFailedException.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace SnowShovel {

// Thin wrapper around CNA's real SDL_Sensor-backed accelerometer.
//
// Port of the XNA original's `#if WINDOWS_PHONE` accelerometer block in
// Game.cs: on the phone, a `ReadingChanged` event handler stashed the latest
// reading, and `UpdateGame` used it to override `accelX`/`accelY` -- taking
// priority over the gamepad/touch/keyboard input that's handled unconditionally
// just above it. On desktop Windows (WINDOWS_PHONE undefined) that whole block
// was compiled out, so keyboard/gamepad/touch was the only input.
//
// CNA's accelerometer is real hardware, not phone-only, so instead of an #if
// this is gated on getIsSupportedProperty() at runtime: when a real
// accelerometer is present, GetTilt() returns a value that overrides
// gamepad/touch/keyboard every frame -- exactly like the phone branch did.
// When absent (the normal desktop case), GetTilt() returns std::nullopt and
// SnowShovelGame's own gamepad/touch/keyboard handling (already present,
// unconditionally, in the original's desktop code path) is used unmodified.
class Accelerometer {
public:
    void Initialize() {
        if (Microsoft::Devices::Sensors::Accelerometer::getIsSupportedProperty()) {
            try {
                sensor_.emplace();
                sensor_->Start();
            } catch (const Microsoft::Devices::Sensors::AccelerometerFailedException&) {
                sensor_.reset();
            }
        }
    }

    // Returns the current raw acceleration vector, or std::nullopt if no
    // accelerometer is present.
    std::optional<Microsoft::Xna::Framework::Vector3> GetTilt() const {
        if (!sensor_.has_value())
            return std::nullopt;
        return sensor_->getCurrentValueProperty().getAccelerationProperty();
    }

private:
    std::optional<Microsoft::Devices::Sensors::Accelerometer> sensor_;
};

} // namespace SnowShovel
