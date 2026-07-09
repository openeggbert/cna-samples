#pragma once

// Ported from ChaseCameraSample.Ship (Ship.cs). Simple flight physics for the ship the chase
// camera follows: thrust along its own forward direction, drag, and free rotation about its
// own local axes (not just world Y), reorthonormalized every frame to avoid drift. Pure
// Vector3/Matrix math plus Keyboard/GamePad/Mouse input -- no CNA API gaps expected or found
// here. The C# original's "touch" helpers (TouchLeft/TouchRight/TouchUp/TouchDown) already
// use MouseState + GraphicsDevice.Viewport thirds directly (this sample predates real XNA
// touch APIs and used mouse-as-touch even on Windows), so they port unchanged -- no
// mouse-substitutes-for-touch adaptation was needed, unlike other samples in this repo.

#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/MouseState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <algorithm>

namespace ChaseCameraSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class Ship {
public:
    // Location of ship in world space.
    Vector3 Position;

    // Direction ship is facing.
    Vector3 Direction;

    // Ship's up vector.
    Vector3 Up;

    // Current ship velocity.
    Vector3 Velocity;

    // Ship's right vector.
    [[nodiscard]] Vector3 Right() const { return right_; }

    // Ship world transform matrix.
    [[nodiscard]] const Matrix& World() const { return world_; }

    // A reference to the graphics device used to access the viewport for touch input.
    explicit Ship(GraphicsDevice& device) : graphicsDevice_(device) { Reset(); }

    // Restore the ship to its original starting state
    void Reset() {
        Position = Vector3(0.0f, MinimumAltitude, 0.0f);
        Direction = Vector3::Forward;
        Up = Vector3::Up;
        right_ = Vector3::Right;
        Velocity = Vector3::Zero;
    }

    // Applies a simple rotation to the ship and animates position based on simple linear
    // motion physics.
    void Update(const GameTime& gameTime) {
        KeyboardState keyboardState = Keyboard::GetState();
        GamePadState gamePadState = GamePad::GetState(PlayerIndex::One);
        MouseState mouseState = Mouse::GetState();

        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // Determine rotation amount from input
        Vector2 rotationAmount = -gamePadState.getThumbSticksProperty().getLeftProperty();
        if (keyboardState.IsKeyDown(Keys::Left) || TouchLeft(mouseState))
            rotationAmount.X = 1.0f;
        if (keyboardState.IsKeyDown(Keys::Right) || TouchRight(mouseState))
            rotationAmount.X = -1.0f;
        if (keyboardState.IsKeyDown(Keys::Up) || TouchUp(mouseState))
            rotationAmount.Y = -1.0f;
        if (keyboardState.IsKeyDown(Keys::Down) || TouchDown(mouseState))
            rotationAmount.Y = 1.0f;

        // Scale rotation amount to radians per second
        rotationAmount = rotationAmount * RotationRate * elapsed;

        // Correct the X axis steering when the ship is upside down
        if (Up.Y < 0)
            rotationAmount.X = -rotationAmount.X;

        // Create rotation matrix from rotation amount
        Matrix rotationMatrix = Matrix::CreateFromAxisAngle(right_, rotationAmount.Y) *
                                 Matrix::CreateRotationY(rotationAmount.X);

        // Rotate orientation vectors
        Direction = Vector3::TransformNormal(Direction, rotationMatrix);
        Up = Vector3::TransformNormal(Up, rotationMatrix);

        // Re-normalize orientation vectors
        // Without this, the matrix transformations may introduce small rounding errors which
        // add up over time and could destabilize the ship.
        Direction.Normalize();
        Up.Normalize();

        // Re-calculate Right
        right_ = Vector3::Cross(Direction, Up);

        // The same instability may cause the 3 orientation vectors to also diverge. Either the
        // Up or Direction vector needs to be re-computed with a cross product to ensure
        // orthogonality
        Up = Vector3::Cross(right_, Direction);

        // Determine thrust amount from input
        float thrustAmount = gamePadState.getTriggersProperty().getRightProperty();
        if (keyboardState.IsKeyDown(Keys::Space) ||
            mouseState.getLeftButtonProperty() == ButtonState::Pressed)
            thrustAmount = 1.0f;

        // Calculate force from thrust amount
        Vector3 force = Direction * thrustAmount * ThrustForce;

        // Apply acceleration
        Vector3 acceleration = force / Mass;
        Velocity += acceleration * elapsed;

        // Apply psuedo drag
        Velocity = Velocity * DragFactor;

        // Apply velocity
        Position += Velocity * elapsed;

        // Prevent ship from flying under the ground
        Position.Y = std::max(Position.Y, MinimumAltitude);

        // Reconstruct the ship's world matrix
        world_ = Matrix::getIdentityProperty();
        world_.setForwardProperty(Direction);
        world_.setUpProperty(Up);
        world_.setRightProperty(right_);
        world_.setTranslationProperty(Position);
    }

private:
    static constexpr float MinimumAltitude = 350.0f;

    // Full speed at which ship can rotate; measured in radians per second.
    static constexpr float RotationRate = 1.5f;

    // Mass of ship.
    static constexpr float Mass = 1.0f;

    // Maximum force that can be applied along the ship's direction.
    static constexpr float ThrustForce = 24000.0f;

    // Velocity scalar to approximate drag.
    static constexpr float DragFactor = 0.97f;

    GraphicsDevice& graphicsDevice_;
    Vector3 right_;
    Matrix world_;

    bool TouchLeft(const MouseState& mouseState) const {
        return mouseState.getLeftButtonProperty() == ButtonState::Pressed &&
               mouseState.getXProperty() <= graphicsDevice_.getViewportProperty().getWidthProperty() / 3;
    }

    bool TouchRight(const MouseState& mouseState) const {
        return mouseState.getLeftButtonProperty() == ButtonState::Pressed &&
               mouseState.getXProperty() >= 2 * graphicsDevice_.getViewportProperty().getWidthProperty() / 3;
    }

    bool TouchDown(const MouseState& mouseState) const {
        return mouseState.getLeftButtonProperty() == ButtonState::Pressed &&
               mouseState.getYProperty() <= graphicsDevice_.getViewportProperty().getHeightProperty() / 3;
    }

    bool TouchUp(const MouseState& mouseState) const {
        return mouseState.getLeftButtonProperty() == ButtonState::Pressed &&
               mouseState.getYProperty() >= 2 * graphicsDevice_.getViewportProperty().getHeightProperty() / 3;
    }
};

} // namespace ChaseCameraSample
