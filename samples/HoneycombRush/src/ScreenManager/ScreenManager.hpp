#pragma once

// ScreenManager.hpp — C++ port of ScreenManager/ScreenManager.cs (XNA 4.0
// HoneycombRush sample). Tombstoning (SerializeState/DeserializeState) and the
// debug TraceScreens() helper are dropped, matching this project's
// established precedent — see missing.md. Adds a shared ButtonBackground
// texture (used by MenuEntry) and registers the shared SpriteBatch as a
// Game.Services entry, since several non-screen DrawableGameComponents
// (ScoreBar, TexturedDrawableGameComponent subclasses) need to reach it.

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

namespace HoneycombRush {

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
// Port of ScreenManager/ScreenManager.cs.
class ScreenManager : public DrawableGameComponent {
public:
    explicit ScreenManager(Game& game) : DrawableGameComponent(game) {
        TouchPanel::setEnabledGesturesProperty(GestureType::None);
    }

    SpriteBatch& getSpriteBatch() { return *spriteBatch_; }
    SpriteFont& getFont() { return *font_; }
    Texture2D& getButtonBackground() { return *buttonBackground_; }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        getGameProperty().getServicesProperty().AddService<SpriteBatch>(spriteBatch_.get());

        auto& content = getGameProperty().getContentProperty();
        font_.emplace(content.Load<SpriteFont>("Fonts/MenuFont"));
        buttonBackground_.emplace(content.Load<Texture2D>("Textures/Backgrounds/buttonBackground"));

        for (auto& screen : screens_)
            screen->LoadContent();

        isInitialized_ = true;
    }

    void UnloadContent() override {
        for (auto& screen : screens_)
            screen->UnloadContent();
    }

    void Update(GameTime& gameTime) override {
        // Actually release screens removed last frame now, i.e. only once no
        // Game::Update()/Draw() component iteration is in flight -- see
        // RemoveScreen() for why this can't happen synchronously.
        pendingDestruction_.clear();

        input_.Update();

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
                    screen->HandleInput(gameTime, input_);
                    otherScreenHasFocus = true;
                }

                if (!screen->IsPopup())
                    coveredByOtherScreen = true;
            }
        }
    }

    void Draw(const GameTime& gameTime) override {
        for (auto& screen : screens_) {
            if (screen->GetScreenState() == ScreenState::Hidden)
                continue;
            screen->Draw(gameTime);
        }
    }

    void AddScreen(std::shared_ptr<GameScreen> screen, std::optional<PlayerIndex> controllingPlayer) {
        screen->setControllingPlayer(controllingPlayer);
        screen->setScreenManager(this);
        screen->setIsExiting(false);

        if (isInitialized_)
            screen->LoadContent();

        screens_.push_back(screen);

        TouchPanel::setEnabledGesturesProperty(screen->EnabledGestures());
    }

    void RemoveScreen(GameScreen* screen) {
        if (isInitialized_)
            screen->UnloadContent();

        // Keep the screen alive until the start of the next Update() call
        // (see pendingDestruction_) instead of letting its refcount drop to
        // zero here. A screen like GameplayScreen can own dozens of
        // GameComponents; destroying them synchronously, mid-frame, while
        // Game::Update()/Draw() are iterating a *snapshot* of Game.Components
        // taken at the top of the frame, leaves that snapshot holding
        // dangling pointers for any not-yet-visited component -- the XNA
        // original never hits this because its GC never frees objects
        // synchronously mid-iteration.
        auto it = std::find_if(screens_.begin(), screens_.end(),
                                [screen](const std::shared_ptr<GameScreen>& s) { return s.get() == screen; });
        if (it != screens_.end()) {
            pendingDestruction_.push_back(std::move(*it));
            screens_.erase(it);
        }

        eraseByPtr(screensToUpdate_, screen);

        if (!screens_.empty()) {
            TouchPanel::setEnabledGesturesProperty(screens_.back()->EnabledGestures());
        }
    }

    std::vector<std::shared_ptr<GameScreen>> GetScreens() const { return screens_; }

private:
    static void eraseByPtr(std::vector<std::shared_ptr<GameScreen>>& v, GameScreen* p) {
        v.erase(std::remove_if(v.begin(), v.end(), [p](const std::shared_ptr<GameScreen>& s) { return s.get() == p; }),
                v.end());
    }

    std::vector<std::shared_ptr<GameScreen>> screens_;
    std::vector<std::shared_ptr<GameScreen>> screensToUpdate_;
    std::vector<std::shared_ptr<GameScreen>> pendingDestruction_;
    InputState input_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;
    std::optional<Texture2D> buttonBackground_;
    bool isInitialized_ = false;
};

// ---- GameScreen methods that depend on ScreenManager (defined here) ----

inline void GameScreen::Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) {
    otherScreenHasFocus_ = otherScreenHasFocus;

    if (isExiting_) {
        screenState_ = ScreenState::TransitionOff;
        if (!UpdateTransition(gameTime, transitionOffTime_, 1)) {
            screenManager_->RemoveScreen(this); // last action: 'this' may be freed
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

} // namespace HoneycombRush
