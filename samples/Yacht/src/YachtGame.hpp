#pragma once

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/DisplayOrientation.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/TimeSpan.hpp"

#include "ScreenManager.hpp"
#include "Screens.hpp"
#include "AudioManager.hpp"
#include "Accelerometer.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::DisplayOrientation;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// The Yacht dice game: a human player takes turns against three AI
// opponents. A port of the XNA 4.0 "Yacht" Windows Phone sample; per the
// approved plan, online multiplayer (WCF service + push notifications) and
// tombstoning (save/load via IsolatedStorageFile) are dropped -- the game
// always starts fresh at the main menu, exactly like CatapultGame.hpp does.
class YachtGame : public Game {
public:
    YachtGame() {
        getContentProperty().setRootDirectoryProperty("Content");

        // The Windows Phone original ran at 30 fps.
        setTargetElapsedTimeProperty(System::TimeSpan::FromSeconds(1.0 / 30.0));

        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setSupportedOrientationsProperty(DisplayOrientation::Portrait);
        graphics_->setPreferredBackBufferWidthProperty(480);
        graphics_->setPreferredBackBufferHeightProperty(800);

        screenManager_ = std::make_unique<ScreenManager>(*this);
        getComponentsProperty().Add(&*screenManager_);

        // Tombstoning/save-load is dropped, so (like CatapultGame.hpp) just
        // construct the main menu directly at startup.
        screenManager_->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);

        AudioManager::Initialize(this);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "YachtGame";
        return name;
    }

protected:
    void Initialize() override {
        // Confirmed gotcha: CNA does not auto-wire the touch panel's display
        // metrics from the back buffer size, so this must be done explicitly.
        TouchPanel::setDisplayWidthProperty(480);
        TouchPanel::setDisplayHeightProperty(800);
        TouchPanel::setDisplayOrientationProperty(DisplayOrientation::Portrait);

        Accelerometer::Initialize();

        Game::Initialize();
    }

    void LoadContent() override {
        AudioManager::LoadSounds();

        // F1 help overlay (CNA addition beyond the XNA original).
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

    std::unique_ptr<SpriteBatch> overlayBatch_;
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_ = false;
};

} // namespace Yacht
