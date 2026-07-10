#pragma once

// MarbleMazeGame.hpp — C++ port of MarbleMazeGame.cs (XNA 4.0 MarbleMaze sample's
// main Game class). Skips forcing fullscreen and never overrides
// PreferredBackBufferWidth/Height, matching CNA's default 800x480 back buffer --
// this repo's DynamicMenu/UISample/HoneycombRush precedent -- which happens to
// exactly match every hardcoded-resolution background image this sample ships
// (titleScreen.png/instructions.png are themselves 800x480).

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/TimeSpan.hpp"

#include "Misc/AudioManager.hpp"
#include "ScreenManager/ScreenManager.hpp"
#include "Screens/ScreensGlue.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Main game class. Port of MarbleMazeGame.cs.
class MarbleMazeGame : public Game {
public:
    MarbleMazeGame() : graphics_(this) {
        AudioManager::Initialize(*this);

        getContentProperty().setRootDirectoryProperty("Content");

        // Frame rate is 30 fps by default for Windows Phone.
        setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333));

        screenManager_ = std::make_shared<ScreenManager>(*this);

        screenManager_->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
        screenManager_->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);

        getComponentsProperty().Add(screenManager_.get());
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "MarbleMazeGame";
        return name;
    }

protected:
    void Initialize() override {
        TouchPanel::setDisplayWidthProperty(GraphicsDeviceManager::DefaultBackBufferWidth);
        TouchPanel::setDisplayHeightProperty(GraphicsDeviceManager::DefaultBackBufferHeight);

        Game::Initialize();
    }

    void LoadContent() override {
        AudioManager::LoadSounds();
        HighScoreScreen::LoadHighscore();

        // F1 help overlay (CNA addition beyond the XNA original -- CLAUDE.md).
        overlayBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        Game::LoadContent();
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        Game::Draw(gameTime);

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);

            overlayBatch_->Begin();
            overlayBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
            overlayBatch_->End();
        }
    }

private:
    GraphicsDeviceManager graphics_;
    std::shared_ptr<ScreenManager> screenManager_;

    std::unique_ptr<SpriteBatch> overlayBatch_;
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace MarbleMazeSample
