#pragma once

// Ported from RimLighting's Arcball.cs (Microsoft Advanced Technology Group). A simple
// arcball class for computing rotations from a user's drag operations on screen. Ported
// nearly verbatim -- see ModelViewerCamera.hpp/RimLightingGame.hpp for how the touch
// input this expects is synthesized from mouse input on desktop (NOXNA, no touchscreen
// on this dev machine -- see missing.md).

#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Quaternion.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"

#include <cmath>

namespace RimLightingSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Input::Touch;

// Simple Arcball class for handling rotations from user's drag operations on screen.
class Arcball {
public:
    // Constructor, the bounding box of the arcball is needed.
    Arcball(int x, int y, int width, int height) {
        offset_ = Vector2(static_cast<float>(x), static_cast<float>(y));
        size_ = Vector2(static_cast<float>(width), static_cast<float>(height));
    }

    // Returns current rotation as quaternion.
    Quaternion GetCurrentRotationQuaternion() const { return qNow_; }

    // Returns current rotation as matrix.
    Matrix GetCurrentRotationMatrix() const { return Matrix::CreateFromQuaternion(qNow_); }

    // Set current rotation using quaternion.
    void SetCurrentRotation(const Quaternion& rotation) { qNow_ = rotation; }

    // Set current rotation using matrix.
    void SetCurrentRotation(const Matrix& rotation) { qNow_ = Quaternion::CreateFromRotationMatrix(rotation); }

    // Is the user currently dragging on this arcball?
    bool IsDragging = false;

    // Process touch input. This should be called within Update() of the game.
    void HandleTouch(const TouchLocation& loc) {
        if (loc.getStateProperty() == TouchLocationState::Pressed && !IsDragging) {
            const Vector2& pos = loc.getPositionProperty();
            if (pos.X >= offset_.X && pos.X < (offset_.X + size_.X) &&
                pos.Y >= offset_.Y && pos.Y < (offset_.Y + size_.Y)) {
                IsDragging = true;
                qDown_ = qNow_;
                downPt_ = ScreenToVector(pos.X, pos.Y);
            }
        } else {
            if (loc.getStateProperty() == TouchLocationState::Released) {
                IsDragging = false;
            }
        }

        if (IsDragging) {
            currentPt_ = ScreenToVector(loc.getPositionProperty().X, loc.getPositionProperty().Y);
            qNow_ = QuatFromBallPoints(downPt_, currentPt_) * qDown_;
        }
    }

private:
    // Converts screen coordinates to coordinates on the sphere.
    Vector3 ScreenToVector(float screenX, float screenY) const {
        // Scale to screen.
        float x = (screenX - offset_.X - size_.X / 2.0f) / (radius_ * size_.X / 2.0f);
        float y = (screenY - offset_.Y - size_.Y / 2.0f) / (radius_ * size_.Y / 2.0f);

        float z = 0.0f;
        float mag = x * x + y * y;

        if (mag > 1.0f) {
            float scale = 1.0f / std::sqrt(mag);
            x *= scale;
            y *= scale;
        } else {
            z = std::sqrt(1.0f - mag);
        }

        return Vector3(x, y, z);
    }

    // Compute rotation quaternion from two points on the sphere.
    static Quaternion QuatFromBallPoints(const Vector3& from, const Vector3& to) {
        float dot = Vector3::Dot(from, to);
        Vector3 qPart = Vector3::Cross(from, to);
        return Quaternion(qPart.X, qPart.Y, qPart.Z, dot);
    }

    // Current rotation quaternion.
    Quaternion qNow_ = Quaternion::Identity;

    // The rotation quaternion when the user clicked down on the screen and began dragging.
    Quaternion qDown_ = Quaternion::Identity;

    // The bounding box in which drag operations are processed by this arcball.
    Vector2 offset_;
    Vector2 size_;

    // Coordinates on the sphere when the user begins dragging.
    Vector3 downPt_;

    // Coordinates on the sphere during dragging.
    Vector3 currentPt_;

    // Sphere radius (proportional to the bounding box).
    float radius_ = 0.9f;
};

} // namespace RimLightingSample
