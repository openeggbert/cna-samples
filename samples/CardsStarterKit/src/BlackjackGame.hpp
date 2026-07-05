#pragma once

// BlackjackGame.hpp -- C++ port of BlackjackGame.cs (XNA 4.0 CardsStarterKit
// sample). The top-level Game class: sets up the GraphicsDeviceManager,
// ScreenManager, and initial screen stack, and initializes AudioManager.
// Also adds the F1 help overlay (CNA addition beyond the XNA original,
// mandatory per this repo's CLAUDE.md) using its own SpriteBatch instance,
// same established pattern as NinjAcademy/HoneycombRush.

#include <memory>
#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

#include "Blackjack/AudioManager.hpp"
#include "Blackjack/Screens/BackgroundScreen.hpp"
#include "Blackjack/Screens/GameplayScreen.hpp"
#include "Blackjack/Screens/InstructionScreen.hpp"
#include "Blackjack/Screens/MainMenuScreen.hpp"
#include "Blackjack/Screens/OptionsMenu.hpp"
#include "Blackjack/Screens/PauseScreen.hpp"
#include "GameStateManagement/ScreenManager.hpp"

namespace Blackjack {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using GameStateManagement::ScreenManager;

class BlackjackGame : public Game {
public:
    BlackjackGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");

        screenManager_ = std::make_shared<ScreenManager>(*this);
        screenManager_->AddScreen(std::make_shared<BackgroundScreen>(), std::nullopt);
        screenManager_->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
        getComponentsProperty().Add(screenManager_.get());

        setIsMouseVisibleProperty(true);

        AudioManager::Initialize(*this);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "BlackjackGame";
        return name;
    }

protected:
    void Initialize() override {
        Game::Initialize();

        graphics_.setPreferredBackBufferHeightProperty(480);
        graphics_.setPreferredBackBufferWidthProperty(800);
        graphics_.ApplyChanges();
    }

    void LoadContent() override {
        AudioManager::LoadSounds();
        Game::LoadContent();

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

} // namespace Blackjack
