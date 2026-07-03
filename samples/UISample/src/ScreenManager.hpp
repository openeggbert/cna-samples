#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"

#include "GameScreen.hpp"

namespace UISample {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// The screen manager is a component which manages one or more GameScreen
// instances. It maintains a stack of screens, calls their Update and Draw
// methods at the appropriate times, and routes input to the topmost screen.
// Port of ScreenManager/ScreenManager.cs. Tombstoning (SerializeState/
// DeserializeState) and the debug TraceScreens() helper are dropped — see
// missing.md.
class ScreenManager : public DrawableGameComponent {
public:
    explicit ScreenManager(Game& game) : DrawableGameComponent(game) {
        // we must set EnabledGestures before we can query for them, but we
        // don't assume the game wants to read them.
        TouchPanel::setEnabledGesturesProperty(GestureType::None);
    }

    SpriteBatch& getSpriteBatch() { return *spriteBatch_; }
    SpriteFont&  getFont()        { return *font_; }
    Texture2D&   getBlankTexture() { return *blankTexture_; }

    // Load content belonging to the screen manager.
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        auto& content = getGameProperty().getContentProperty();
        font_.emplace(content.Load<SpriteFont>("Font/MenuTitle"));

        blankTexture_.emplace(getGraphicsDeviceProperty(), 1, 1);
        Color white = Color::White;
        blankTexture_->SetData(&white, 1);

        for (auto& screen : screens_)
            screen->LoadContent();

        isInitialized_ = true;
    }

    void UnloadContent() override {
        for (auto& screen : screens_)
            screen->UnloadContent();
    }

    // Allows each screen to run logic.
    void Update(GameTime& gameTime) override {
        input_.Update();

        // Make a copy of the master screen list, to avoid confusion if the
        // process of updating one screen adds or removes others.
        screensToUpdate_.clear();
        for (auto& screen : screens_)
            screensToUpdate_.push_back(screen);

        bool otherScreenHasFocus = !getGameProperty().getIsActiveProperty();
        bool coveredByOtherScreen = false;

        while (!screensToUpdate_.empty()) {
            // Pop the topmost screen off the waiting list.
            std::shared_ptr<GameScreen> screen = screensToUpdate_.back();
            screensToUpdate_.pop_back();

            screen->Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            if (screen->GetScreenState() == ScreenState::TransitionOn ||
                screen->GetScreenState() == ScreenState::Active) {
                // Give the first active screen a chance to handle input.
                if (!otherScreenHasFocus) {
                    screen->HandleInput(input_);
                    otherScreenHasFocus = true;
                }

                // An active non-popup covers any subsequent screens.
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

        screens_.push_back(screen);

        // update the TouchPanel to respond to gestures this screen is interested in
        TouchPanel::setEnabledGesturesProperty(screen->EnabledGestures());
    }

    // Removes a screen from the screen manager. Normally use GameScreen::ExitScreen
    // instead, so the screen can transition off rather than being instantly removed.
    void RemoveScreen(GameScreen* screen) {
        if (isInitialized_)
            screen->UnloadContent();

        eraseByPtr(screens_, screen);
        eraseByPtr(screensToUpdate_, screen);

        // if there is a screen still in the manager, update TouchPanel to
        // respond to gestures that screen is interested in.
        if (!screens_.empty()) {
            TouchPanel::setEnabledGesturesProperty(screens_.back()->EnabledGestures());
        }
    }

    // Returns a copy of the screen list (callers must only mutate via Add/Remove).
    std::vector<std::shared_ptr<GameScreen>> GetScreens() const {
        return screens_;
    }

    // Draws a translucent black fullscreen sprite, used for fading screens in and
    // out and for darkening the background behind popups.
    void FadeBackBufferToBlack(float alpha) {
        auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        spriteBatch_->Begin();
        spriteBatch_->Draw(*blankTexture_,
                           Rectangle(0, 0, viewport.getWidthProperty(), viewport.getHeightProperty()),
                           mul(Color::Black, alpha));
        spriteBatch_->End();
    }

private:
    static void eraseByPtr(std::vector<std::shared_ptr<GameScreen>>& v, GameScreen* p) {
        v.erase(std::remove_if(v.begin(), v.end(),
                               [p](const std::shared_ptr<GameScreen>& s) { return s.get() == p; }),
                v.end());
    }

    std::vector<std::shared_ptr<GameScreen>> screens_;
    std::vector<std::shared_ptr<GameScreen>> screensToUpdate_;
    InputState input_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;
    std::optional<Texture2D> blankTexture_;
    bool isInitialized_ = false;
};

// ---- GameScreen methods that depend on ScreenManager (defined here) ----

inline void GameScreen::Update(GameTime& gameTime, bool otherScreenHasFocus,
                               bool coveredByOtherScreen) {
    otherScreenHasFocus_ = otherScreenHasFocus;

    if (isExiting_) {
        // The screen is going away to die, so it should transition off.
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

} // namespace UISample
