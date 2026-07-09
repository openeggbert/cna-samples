#pragma once

// Ported from ChaseCameraSample.ChaseCamera (ChaseCamera.cs). A simple physics-driven chase
// camera: the camera is treated as a point mass attached to its "desired" position (relative
// to the chased object) by a damped spring. Pure Vector3/Matrix math, no CNA API surface
// beyond what every other 3D sample already uses -- no CNA gaps expected or found here.
//
// C# properties become plain public fields for the read/write ones (matching this repo's own
// established style for sample-level, non-CNA-framework classes -- see Cat.hpp/HeightMapInfo.hpp)
// and small no-argument methods for the read-only/computed ones (Position(), Velocity(),
// View(), Projection(), DesiredPosition(), LookAt()).

#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

namespace ChaseCameraSample {

using namespace Microsoft::Xna::Framework;

class ChaseCamera {
public:
    // --- Chased object properties (set externally each frame) ---

    // Position of object being chased.
    Vector3 ChasePosition;

    // Direction the chased object is facing.
    Vector3 ChaseDirection;

    // Chased object's Up vector.
    Vector3 Up = Vector3::Up;

    // --- Desired camera positioning (set when creating camera or changing view) ---

    // Desired camera position in the chased object's coordinate system.
    Vector3 DesiredPositionOffset = Vector3(0.0f, 2.0f, 2.0f);

    // Look at point in the chased object's coordinate system.
    Vector3 LookAtOffset = Vector3(0.0f, 2.8f, 0.0f);

    // --- Camera physics (typically set when creating camera) ---

    // Physics coefficient which controls the influence of the camera's position over the
    // spring force. The stiffer the spring, the closer it will stay to the chased object.
    float Stiffness = 1800.0f;

    // Physics coefficient which approximates internal friction of the spring. Sufficient
    // damping will prevent the spring from oscillating infinitely.
    float Damping = 600.0f;

    // Mass of the camera body. Heavier objects require stiffer springs with less damping to
    // move at the same rate as lighter objects.
    float Mass = 50.0f;

    // --- Perspective properties ---

    // Perspective aspect ratio. Default value should be overridden by application.
    float AspectRatio = 4.0f / 3.0f;

    // Perspective field of view.
    float FieldOfView = MathHelper::ToRadians(45.0f);

    // Distance to the near clipping plane.
    float NearPlaneDistance = 1.0f;

    // Distance to the far clipping plane.
    float FarPlaneDistance = 100000.0f;

    // --- Desired camera positioning (read-only, computed) ---

    // Desired camera position in world space. Ensures correct value even if Update has not
    // been called this frame, matching the C# original's own defensive getter.
    Vector3 DesiredPosition() {
        UpdateWorldPositions();
        return desiredPosition_;
    }

    // Look at point in world space.
    Vector3 LookAt() {
        UpdateWorldPositions();
        return lookAt_;
    }

    // --- Current camera properties (read-only, updated by camera physics) ---

    // Position of camera in world space.
    [[nodiscard]] Vector3 Position() const { return position_; }

    // Velocity of camera.
    [[nodiscard]] Vector3 Velocity() const { return velocity_; }

    // --- Matrix properties (read-only, updated by camera physics) ---

    // View transform matrix.
    [[nodiscard]] const Matrix& View() const { return view_; }

    // Projection transform matrix.
    [[nodiscard]] const Matrix& Projection() const { return projection_; }

    // --- Methods ---

    // Forces camera to be at desired position and to stop moving. This is useful when the
    // chased object is first created or after it has been teleported. Failing to call this
    // after a large change to the chased object's position will result in the camera quickly
    // flying across the world.
    void Reset() {
        UpdateWorldPositions();

        // Stop motion
        velocity_ = Vector3::Zero;

        // Force desired position
        position_ = desiredPosition_;

        UpdateMatrices();
    }

    // Animates the camera from its current position towards the desired offset behind the
    // chased object. The camera's animation is controlled by a simple physical spring
    // attached to the camera and anchored to the desired position.
    void Update(const GameTime& gameTime) {
        UpdateWorldPositions();

        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // Calculate spring force
        Vector3 stretch = position_ - desiredPosition_;
        Vector3 force = -Stiffness * stretch - Damping * velocity_;

        // Apply acceleration
        Vector3 acceleration = force / Mass;
        velocity_ += acceleration * elapsed;

        // Apply velocity
        position_ += velocity_ * elapsed;

        UpdateMatrices();
    }

private:
    Vector3 desiredPosition_;
    Vector3 lookAt_;
    Vector3 position_;
    Vector3 velocity_;
    Matrix view_;
    Matrix projection_;

    // Rebuilds object space values in world space. Invoke before publicly returning or
    // privately accessing world space values.
    void UpdateWorldPositions() {
        // Construct a matrix to transform from object space to worldspace
        Matrix transform = Matrix::getIdentityProperty();
        transform.setForwardProperty(ChaseDirection);
        transform.setUpProperty(Up);
        transform.setRightProperty(Vector3::Cross(Up, ChaseDirection));

        // Calculate desired camera properties in world space
        desiredPosition_ = ChasePosition + Vector3::TransformNormal(DesiredPositionOffset, transform);
        lookAt_ = ChasePosition + Vector3::TransformNormal(LookAtOffset, transform);
    }

    // Rebuilds camera's view and projection matrices.
    void UpdateMatrices() {
        view_ = Matrix::CreateLookAt(position_, lookAt_, Up);
        projection_ = Matrix::CreatePerspectiveFieldOfView(FieldOfView, AspectRatio,
                                                            NearPlaneDistance, FarPlaneDistance);
    }
};

} // namespace ChaseCameraSample
