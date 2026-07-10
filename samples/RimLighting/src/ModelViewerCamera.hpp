#pragma once

// Ported from RimLighting's ModelViewerCamera.cs (Microsoft Advanced Technology Group).
// A simple model-viewer camera that uses two arcballs: one for rotating the object in
// world space, and one for rotating the camera around the object. Ported verbatim.

#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"

#include "Arcball.hpp"

namespace RimLightingSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Input::Touch;

class ModelViewerCamera {
public:
    // Constructor: camera position, camera up direction, and the bounding box of the
    // arcball are needed.
    ModelViewerCamera(const Vector3& cameraPosition, const Vector3& cameraUpDir,
                       int x, int y, int width, int height)
        : cameraPosition_(cameraPosition), cameraUpDir_(cameraUpDir),
          worldArcball_(x, y, width, height), viewArcball_(x, y, width, height) {}

    // Mode selector: is currently rotating in world space, or rotating camera?
    // True when rotating world, false when rotating camera.
    bool GetIsRotatingWorld() const { return isRotatingWorld_; }
    void SetIsRotatingWorld(bool value) {
        if (!isRotatingWorld_ && value) {
            // Absorb the difference from last view rotation to current view rotation
            // into world rotation.
            worldArcball_.SetCurrentRotation(
                Matrix::Invert(viewArcball_.GetCurrentRotationMatrix()) * lastViewRotation_ *
                worldArcball_.GetCurrentRotationMatrix() *
                Matrix::Invert(lastViewRotation_) * viewArcball_.GetCurrentRotationMatrix());

            // So that when lastViewRotation is updated here, GetWorldMatrix() still
            // returns the same world rotation. This is necessary since we don't want
            // the object to jump when switching between world/camera rotation modes.
            lastViewRotation_ = viewArcball_.GetCurrentRotationMatrix();
        }

        isRotatingWorld_ = value;
    }

    // Process the touch input, rotates the world or camera according to current mode.
    void HandleTouch(const TouchLocation& loc) {
        if (isRotatingWorld_)
            worldArcball_.HandleTouch(loc);
        else
            viewArcball_.HandleTouch(loc);
    }

    // Get current world matrix.
    Matrix GetWorldMatrix() const {
        // V * W * V^-1 is needed here because we want the object to rotate naturally
        // no matter whether the rotation of the view matrix is identity or not.
        return lastViewRotation_ * worldArcball_.GetCurrentRotationMatrix() * Matrix::Invert(lastViewRotation_);
    }

    // Get current view matrix.
    Matrix GetViewMatrix() const {
        // Rotate the camera.
        Matrix rot = Matrix::Invert(viewArcball_.GetCurrentRotationMatrix());
        return Matrix::CreateLookAt(Vector3::Transform(cameraPosition_, rot), Vector3::Zero,
                                     Vector3::Transform(cameraUpDir_, rot));
    }

    Arcball& GetWorldArcball() { return worldArcball_; }
    Arcball& GetViewArcball() { return viewArcball_; }

private:
    bool isRotatingWorld_ = true;

    // Arcballs for rotating world and camera.
    Arcball worldArcball_;
    Arcball viewArcball_;

    // The initial camera position and up direction.
    Vector3 cameraPosition_;
    Vector3 cameraUpDir_;

    Matrix lastViewRotation_ = Matrix::getIdentityProperty();
};

} // namespace RimLightingSample
