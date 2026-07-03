#pragma once

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/TimeSpan.hpp"

#include "ScreenManager.hpp"
#include "Screens.hpp"

namespace UISample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// This sample builds on the GameStateManagement sample, adding UI elements
// with a look and feel similar to Windows Phone 7 Silverlight applications:
// TextControl, ImageControl, PanelControl, ScrollingPanelControl, and
// PageFlipControl. Port of the XNA 4.0 "User Interface Controls Sample"
// (SampleGame.cs). Tombstoning (DeserializeState) is dropped, matching
// Yacht's precedent — see missing.md.
class UISampleGame : public Game {
public:
    UISampleGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");

        // Frame rate is 30 fps by default for Windows Phone.
        setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333));

        screenManager_ = std::make_unique<ScreenManager>(*this);
        getComponentsProperty().Add(&*screenManager_);

        screenManager_->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
        screenManager_->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "UISampleGame";
        return name;
    }

protected:
    void Initialize() override {
        // Gotcha (see missing.md/NEXT.md): CNA does not auto-wire the touch
        // panel's display metrics from the back buffer size, so this must be
        // done explicitly, or PageFlipTracker/ScrollTracker (which both read
        // TouchPanel::DisplayWidth/Height) compute against 0. This sample
        // never overrides PreferredBackBufferWidth/Height, relying on CNA's
        // default (800x480, landscape, matching the original's implicit
        // default per the Levels/*.png art being native 800x480) -- so the
        // known default constants are used directly here rather than
        // querying getViewportProperty(), which reads stale this early (see
        // SnowShovel's missing.md for the same gotcha in more detail).
        TouchPanel::setDisplayWidthProperty(GraphicsDeviceManager::DefaultBackBufferWidth);
        TouchPanel::setDisplayHeightProperty(GraphicsDeviceManager::DefaultBackBufferHeight);

        Game::Initialize();
    }

    void LoadContent() override {
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

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::Black);
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
    std::unique_ptr<ScreenManager> screenManager_;

    std::unique_ptr<SpriteBatch> overlayBatch_;
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace UISample
