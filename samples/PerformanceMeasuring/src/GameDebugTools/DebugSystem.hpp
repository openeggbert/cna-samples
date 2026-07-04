#pragma once

// DebugSystem.hpp — C++ port of GameDebugTools/DebugSystem.cs (XNA 4.0
// PerformanceMeasuring sample). Streamlines creation of the GameDebugTools
// pieces (DebugManager, DebugCommandUI, FpsCounter, TimeRuler) and adds them
// to the game's Components collection.
//
// Adaptation note: the original's RemoteDebugCommand member (Xbox 360
// SystemLink remote console) is omitted — see DebugCommandUI.hpp/missing.md.

#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Game.hpp"

#include "DebugCommandUI.hpp"
#include "DebugManager.hpp"
#include "FpsCounter.hpp"
#include "TimeRuler.hpp"

namespace PerformanceMeasuring::GameDebugTools {

using Microsoft::Xna::Framework::Game;

// Helper class that streamlines the creation of the GameDebugTools pieces.
// Port of GameDebugTools/DebugSystem.cs.
class DebugSystem {
public:
    static DebugSystem& Initialize(Game& game, const std::string& debugFont) {
        if (instance_ != nullptr)
            return *instance_;

        instance_ = std::unique_ptr<DebugSystem>(new DebugSystem());

        instance_->debugManager_ = std::make_shared<DebugManager>(game, debugFont);
        game.getComponentsProperty().Add(instance_->debugManager_.get());

        instance_->debugCommandUI_ = std::make_shared<DebugCommandUI>(game);
        game.getComponentsProperty().Add(instance_->debugCommandUI_.get());

        instance_->fpsCounter_ = std::make_shared<FpsCounter>(game);
        game.getComponentsProperty().Add(instance_->fpsCounter_.get());

        instance_->timeRuler_ = std::make_shared<TimeRuler>(game);
        game.getComponentsProperty().Add(instance_->timeRuler_.get());

        return *instance_;
    }

    static DebugSystem& Instance() { return *instance_; }

    DebugManager& getDebugManager() { return *debugManager_; }
    DebugCommandUI& getDebugCommandUI() { return *debugCommandUI_; }
    FpsCounter& getFpsCounter() { return *fpsCounter_; }
    TimeRuler& getTimeRuler() { return *timeRuler_; }

private:
    DebugSystem() = default;

    static inline std::unique_ptr<DebugSystem> instance_;

    // Owned by DebugSystem so their lifetime outlives Game::Components' raw
    // pointers; the components themselves are non-owning-registered there
    // (GameComponentCollection stores IGameComponent* without taking ownership).
    std::shared_ptr<DebugManager> debugManager_;
    std::shared_ptr<DebugCommandUI> debugCommandUI_;
    std::shared_ptr<FpsCounter> fpsCounter_;
    std::shared_ptr<TimeRuler> timeRuler_;
};

} // namespace PerformanceMeasuring::GameDebugTools
