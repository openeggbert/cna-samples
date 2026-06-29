#pragma once

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

#include "ScreenManager.hpp"
#include "Screens.hpp"

namespace GameStateManagement {

using Microsoft::Xna::Framework::GraphicsDeviceManager;

// Sample showing how to manage different game states, with transitions between
// menu screens, a loading screen, the game itself, and a pause menu. The main
// game class is extremely simple: all the interesting work happens in the
// ScreenManager component.
class GameStateManagementGame : public Game {
public:
    GameStateManagementGame() {
        getContentProperty().setRootDirectoryProperty("Content");

        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(853);
        graphics_->setPreferredBackBufferHeightProperty(480);

        // Create the screen manager component.
        screenManager_ = std::make_unique<ScreenManager>(*this);
        getComponentsProperty().Add(&*screenManager_);

        // Activate the first screens.
        screenManager_->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
        screenManager_->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "GameStateManagementGame";
        return name;
    }

protected:
    void LoadContent() override {
        // Preload UI assets to avoid framerate glitches during transitions.
        gradientPreload_.emplace(getContentProperty().Load<Texture2D>("gradient"));

        // F1 help overlay (CNA addition beyond the XNA original).
        overlayBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        Game::Update(gameTime);  // updates the ScreenManager component
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::Black);

        // The real drawing happens inside the screen manager component.
        Game::Draw(gameTime);

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            overlayBatch_->Begin();
            overlayBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
            overlayBatch_->End();
        }
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::unique_ptr<ScreenManager> screenManager_;

    std::optional<Texture2D> gradientPreload_;
    std::unique_ptr<SpriteBatch> overlayBatch_;
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_ = false;
};

} // namespace GameStateManagement
