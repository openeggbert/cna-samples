#pragma once

// ScreenManager.hpp -- C++ port of ScreenManager/ScreenManager.cs (XNA 4.0
// CardsStarterKit sample). Adds SafeArea/ButtonBackground beyond the stock
// "Game State Management" template (already ported in samples/
// GameStateManagement and samples/NinjAcademy) to support this sample's
// button-background-driven menu rendering. IsolatedStorageFile-based
// SerializeState()/DeserializeState() are dropped -- see missing.md, same
// precedent as every other ScreenManager port in this repo.

#include <algorithm>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "GameScreen.hpp"

namespace GameStateManagement {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// The screen manager is a component which manages one or more GameScreen
// instances. It maintains a stack of screens, calls their Update and Draw
// methods at the appropriate times, and routes input to the topmost screen.
class ScreenManager : public DrawableGameComponent {
public:
    InputState input;

    explicit ScreenManager(Game& game) : DrawableGameComponent(game) {}

    SpriteBatch& getSpriteBatch() { return *spriteBatch_; }
    SpriteFont&  getFont()        { return *font_; }
    Texture2D&   getButtonBackground() { return *buttonBackground_; }
    Texture2D&   getBlankTexture() { return *blankTexture_; }

    // Returns the portion of the screen where drawing is safely allowed.
    Rectangle SafeArea() {
        return getGraphicsDeviceProperty().getViewportProperty().getTitleSafeAreaProperty();
    }

    // Load content belonging to the screen manager.
    void LoadContent() override {
        auto& content = getGameProperty().getContentProperty();

        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        font_.emplace(content.Load<SpriteFont>("Fonts/MenuFont"));
        blankTexture_.emplace(content.Load<Texture2D>("Images/blank"));
        buttonBackground_.emplace(content.Load<Texture2D>("Images/ButtonRegular"));

        for (auto& screen : screens_)
            screen->LoadContent();
    }

    void UnloadContent() override {
        for (auto& screen : screens_)
            screen->UnloadContent();
    }

    // Allows each screen to run logic.
    void Update(GameTime& gameTime) override {
        input.Update();

        // Make a copy of the master screen list, to avoid confusion if the
        // process of updating one screen adds or removes others.
        screensToUpdate_.clear();
        for (auto& screen : screens_)
            screensToUpdate_.push_back(screen);

        bool otherScreenHasFocus = !getGameProperty().getIsActiveProperty();
        bool coveredByOtherScreen = false;

        while (!screensToUpdate_.empty()) {
            std::shared_ptr<GameScreen> screen = screensToUpdate_.back();
            screensToUpdate_.pop_back();

            screen->Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            if (screen->GetScreenState() == ScreenState::TransitionOn ||
                screen->GetScreenState() == ScreenState::Active) {
                if (!otherScreenHasFocus) {
                    screen->HandleInput(input);
                    otherScreenHasFocus = true;
                }
                if (!screen->IsPopup())
                    coveredByOtherScreen = true;
            }
        }
    }

    // Tells each screen to draw itself.
    void Draw(const GameTime& gameTime) override {
        for (auto& screen : screens_) {
            if (screen->GetScreenState() == ScreenState::Hidden)
                continue;
            screen->Draw(gameTime);
        }
    }

    // Adds a new screen to the screen manager.
    void AddScreen(std::shared_ptr<GameScreen> screen,
                   std::optional<PlayerIndex> controllingPlayer) {
        screen->setControllingPlayer(controllingPlayer);
        screen->setScreenManager(this);
        screen->setIsExiting(false);

        if (isInitialized_)
            screen->LoadContent();

        screens_.push_back(std::move(screen));
    }

    // Removes a screen from the screen manager. Normally use GameScreen::ExitScreen
    // instead, so the screen can transition off rather than being instantly removed.
    void RemoveScreen(GameScreen* screen) {
        if (isInitialized_)
            screen->UnloadContent();

        eraseByPtr(screens_, screen);
        eraseByPtr(screensToUpdate_, screen);
    }

    // Returns a copy of the screen list (callers must only mutate via Add/Remove).
    std::vector<std::shared_ptr<GameScreen>> GetScreens() const {
        return screens_;
    }

    // Draws a translucent black fullscreen sprite, used for fading screens in
    // and out and for darkening the background behind popups.
    void FadeBackBufferToBlack(float alpha) {
        auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        spriteBatch_->Begin();
        spriteBatch_->Draw(*blankTexture_,
                           Rectangle(0, 0, viewport.getWidthProperty(), viewport.getHeightProperty()),
                           mul(Color::Black, alpha));
        spriteBatch_->End();
    }

    void Initialize() override {
        DrawableGameComponent::Initialize();
        isInitialized_ = true;
    }

private:
    static void eraseByPtr(std::vector<std::shared_ptr<GameScreen>>& v, GameScreen* p) {
        v.erase(std::remove_if(v.begin(), v.end(),
                               [p](const std::shared_ptr<GameScreen>& s) { return s.get() == p; }),
                v.end());
    }

    std::vector<std::shared_ptr<GameScreen>> screens_;
    std::vector<std::shared_ptr<GameScreen>> screensToUpdate_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;
    std::optional<Texture2D> blankTexture_;
    std::optional<Texture2D> buttonBackground_;
    bool isInitialized_ = false;
};

// ---- GameScreen methods that depend on ScreenManager (defined here) ----

inline void GameScreen::Update(GameTime& gameTime, bool otherScreenHasFocus,
                               bool coveredByOtherScreen) {
    otherScreenHasFocus_ = otherScreenHasFocus;

    if (isExiting_) {
        screenState_ = ScreenState::TransitionOff;
        if (!UpdateTransition(gameTime, transitionOffTime_, 1)) {
            screenManager_->RemoveScreen(this);  // last action: 'this' may be freed
        }
    } else if (coveredByOtherScreen) {
        if (UpdateTransition(gameTime, transitionOffTime_, 1))
            screenState_ = ScreenState::TransitionOff;
        else
            screenState_ = ScreenState::Hidden;
    } else {
        if (UpdateTransition(gameTime, transitionOnTime_, -1))
            screenState_ = ScreenState::TransitionOn;
        else
            screenState_ = ScreenState::Active;
    }
}

inline void GameScreen::ExitScreen() {
    if (transitionOffTime_.getTotalMillisecondsProperty() <= 0.0) {
        screenManager_->RemoveScreen(this);
    } else {
        isExiting_ = true;
    }
}

template <typename T>
inline T GameScreen::Load(const std::string& assetName) {
    return screenManager_->getGameProperty().getContentProperty().Load<T>(assetName);
}

} // namespace GameStateManagement
