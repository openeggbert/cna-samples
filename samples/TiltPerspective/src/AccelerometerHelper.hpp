#pragma once

// Port of AccelerometerHelper.cs (XNA 4.0 TiltPerspective sample).
//
// IMPORTANT DIFFERENCE FROM THE C# ORIGINAL -- read before touching this file.
//
// The C# original wraps a real Microsoft.Devices.Sensors.Accelerometer
// unconditionally (no `#if WINDOWS_PHONE` split at the file/class level --
// confirmed by direct read; TiltPerspective.csproj only defines a
// `Windows Phone` configuration). At Initialize() time it only tries to
// Start() the real sensor when
// `Microsoft.Devices.Environment.DeviceType == DeviceType.Device` (real
// hardware); otherwise (including "running in the emulator", the only case
// that matters for a desktop port) it sets `Sensor = null` and, every
// Update(), synthesizes a purely time-driven, NON-interactive wobble:
//
//     FakeRollTheta += elapsedSeconds * FakeRollSpeed;   // auto-rotates forever
//     RawAcceleration = new Vector3(
//         sin(FakeRollPhi) * cos(FakeRollTheta),
//         sin(FakeRollPhi) * sin(FakeRollTheta),
//         -cos(FakeRollPhi));
//     SmoothAcceleration = RawAcceleration;   // no smoothing at all on this path
//
// There is NO keyboard/gamepad branch anywhere in this file to "un-#if" the
// way AccelerometerSample's/Yacht's/SnowShovel's/Bounce's own fallbacks were
// (DEFERRED.md item #15) -- this is confirmed by a full read of the file:
// no `Keyboard`/`GamePad` symbol appears anywhere in AccelerometerHelper.cs.
// So, per the 2026-07-10 user go/no-go for this specific sample, this port
// INVENTS a keyboard-tilt control scheme from scratch (NOXNA) rather than
// promoting an existing branch:
//
//   Left/Right arrow -> tilt X (device "roll")
//   Up/Down arrow    -> tilt Y (device "pitch")
//   Z is held at -1 (matching the original's own initial/rest value,
//   Vector3(0, 0, -1) -- "gravity straight down through the back of the
//   device" when not tilted), then the whole vector is Normalize()'d.
//
// This exact X/Y-arrow-key/Z=-1/Normalize() shape is not arbitrary -- it is
// the same convention this repo already ported literally from
// AccelerometerSample's/Yacht's own C# *emulator* fallback branches (see
// samples/AccelerometerSample/src/Accelerometer.hpp,
// samples/Yacht/src/Accelerometer.hpp), reused here for consistency across
// the repo's tilt-emulation samples even though, unlike those two, nothing
// in TiltPerspective's own original resembles it -- see missing.md for the
// full rationale.
//
// Everything downstream (BallSimulation.hpp, TiltPerspectiveGame.hpp) reads
// only RawAcceleration/SmoothAcceleration, exactly like the C# original, so
// none of that code needed to change -- only the *source* of these two
// vectors changed, from a sinusoidal timer to keyboard state.

#include "Microsoft/Xna/Framework/GameComponent.hpp"
#include "Microsoft/Xna/Framework/GameComponentCollection.hpp"
#include "Microsoft/Xna/Framework/GameServiceContainer.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

namespace TiltPerspectiveSample {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameComponent;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;

// Mirrors the C# original's IAccelerometerService interface, registered in
// Game.Services so other components (BallSimulation) can look it up without
// a direct reference to the concrete AccelerometerHelper type.
class IAccelerometerService {
public:
    virtual ~IAccelerometerService() = default;

    [[nodiscard]] virtual Vector3 getRawAccelerationProperty() const = 0;
    [[nodiscard]] virtual Vector3 getSmoothAccelerationProperty() const = 0;
};

// Port of the AccelerometerHelper GameComponent. See the file-level comment
// above for the one genuinely invented piece (the keyboard-tilt synthesis in
// Update()) versus what is a faithful, structural port of the original.
class AccelerometerHelper : public GameComponent, public IAccelerometerService {
public:
    explicit AccelerometerHelper(Game& game) : GameComponent(game) {
        game.getServicesProperty().AddService<IAccelerometerService>(this);
    }

    void Initialize() override {
        rawAcceleration_ = Vector3(0.0f, 0.0f, -1.0f);
        smoothAcceleration_ = Vector3(0.0f, 0.0f, -1.0f);
        GameComponent::Initialize();
    }

    void Update(GameTime& gameTime) override {
        (void)gameTime;

        // NOXNA: keyboard-tilt emulation, invented for this port -- see the
        // file-level comment above. This replaces the original's own
        // Sensor==null fallback (a non-interactive sinusoidal wobble); it
        // does NOT replace any real-hardware path, since this desktop build
        // has none.
        KeyboardState keyboardState = Keyboard::GetState();

        Vector3 stateValue;
        stateValue.Z = -1.0f;

        if (keyboardState.IsKeyDown(Keys::Left))
            stateValue.X -= 1.0f;
        if (keyboardState.IsKeyDown(Keys::Right))
            stateValue.X += 1.0f;
        if (keyboardState.IsKeyDown(Keys::Up))
            stateValue.Y += 1.0f;
        if (keyboardState.IsKeyDown(Keys::Down))
            stateValue.Y -= 1.0f;

        stateValue.Normalize();

        rawAcceleration_ = stateValue;
        // Matches the original's own Sensor==null path exactly: SmoothAcceleration
        // is just assigned RawAcceleration, with no lerp/smoothing at all.
        smoothAcceleration_ = stateValue;

        GameComponent::Update(gameTime);
    }

    [[nodiscard]] Vector3 getRawAccelerationProperty() const override { return rawAcceleration_; }
    [[nodiscard]] Vector3 getSmoothAccelerationProperty() const override { return smoothAcceleration_; }

private:
    Vector3 rawAcceleration_{0.0f, 0.0f, -1.0f};
    Vector3 smoothAcceleration_{0.0f, 0.0f, -1.0f};
};

} // namespace TiltPerspectiveSample
