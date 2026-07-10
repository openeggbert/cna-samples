#include "CNA/Entrypoint.hpp"

#include "DemoGame.hpp"
#include "TitleMenu.hpp"
#include "BasicDemo.hpp"
#include "DualDemo.hpp"
#include "AlphaDemo.hpp"
#include "SkinnedDemo.hpp"
#include "EnvmapDemo.hpp"
#include "ParticleDemo.hpp"

// DemoGame's constructor is defined here (not inline in DemoGame.hpp) because it needs
// to construct all 7 concrete MenuComponent subclasses above, each of which itself
// `#include`s DemoGame.hpp -- see DemoGame.hpp's own top-of-file comment for the full
// reasoning (the same header-split this repo's GameStateManagement port already
// established for GameScreen.hpp/ScreenManager.hpp).
namespace ReachGraphicsDemoSample {

DemoGame::DemoGame() {
    getContentProperty().setRootDirectoryProperty("Content");

    graphics_ = std::make_unique<GraphicsDeviceManager>(this);

    graphics_->setPreferredBackBufferWidthProperty(480);
    graphics_->setPreferredBackBufferHeightProperty(800);

    setIsMouseVisibleProperty(true);

    setTargetElapsedTimeProperty(System::TimeSpan::FromSeconds(1.0 / 30.0));

    // Create all the different menu screens (same order as the C# original --
    // TitleMenu's own entries call SetActiveMenu(1..6) matching these indices exactly).
    menuComponents_.push_back(std::make_shared<TitleMenu>(*this));
    menuComponents_.push_back(std::make_shared<BasicDemo>(*this));
    menuComponents_.push_back(std::make_shared<DualDemo>(*this));
    menuComponents_.push_back(std::make_shared<AlphaDemo>(*this));
    menuComponents_.push_back(std::make_shared<SkinnedDemo>(*this));
    menuComponents_.push_back(std::make_shared<EnvmapDemo>(*this));
    menuComponents_.push_back(std::make_shared<ParticleDemo>(*this));

    // Set all the menu screens except the first to hidden and inactive.
    for (auto& component : menuComponents_) {
        component->setEnabledProperty(false);
        component->setVisibleProperty(false);

        getComponentsProperty().Add(component.get());
    }

    // Make the title menu active and visible.
    menuComponents_[0]->setEnabledProperty(true);
    menuComponents_[0]->setVisibleProperty(true);
}

} // namespace ReachGraphicsDemoSample

int main() {
    ReachGraphicsDemoSample::DemoGame game;
    game.Run();
    return 0;
}
