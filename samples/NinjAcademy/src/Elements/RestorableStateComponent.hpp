#pragma once

// RestorableStateComponent.hpp — C++ port of Elements/RestorableStateComponent.cs
// (XNA 4.0 NinjAcademy sample). A drawable game component which can
// remember and restore its Visible/Enabled state (used when pausing).

#include <stdexcept>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;

// A drawable game component which can remember and restore its state. Port
// of Elements/RestorableStateComponent.cs.
class RestorableStateComponent : public DrawableGameComponent {
public:
    explicit RestorableStateComponent(Game& game) : DrawableGameComponent(game) {}

    // Stores the component's visibility and whether or not it is enabled.
    void StoreState() {
        stateStored_ = true;
        wasVisible_ = getVisibleProperty();
        wasEnabled_ = getEnabledProperty();
    }

    // Restores the component's state after it has been stored.
    void RestoreState() {
        if (!stateStored_) {
            throw std::logic_error("Cannot restore the current state before storing it first.");
        }

        stateStored_ = false;

        setVisibleProperty(wasVisible_);
        setEnabledProperty(wasEnabled_);
    }

private:
    bool stateStored_ = false;
    bool wasVisible_ = false;
    bool wasEnabled_ = false;
};

} // namespace NinjAcademy
