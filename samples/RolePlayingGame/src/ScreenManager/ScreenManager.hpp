#pragma once

// ScreenManager.hpp -- C++ port of ScreenManager/ScreenManager.cs. No shared
// font/blank-texture here (unlike other ScreenManager ports in this repo) --
// the original never had them either; each screen loads its own content.

#include <algorithm>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"

#include "GameScreen.hpp"

namespace RolePlaying {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;

class ScreenManager : public DrawableGameComponent {
public:
    explicit ScreenManager(Game& game) : DrawableGameComponent(game) {}

    SpriteBatch& getSpriteBatch() { return *spriteBatch_; }

    void Initialize() override {
        DrawableGameComponent::Initialize();
        isInitialized_ = true;
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        for (auto& screen : screens_) screen->LoadContent();
    }

    void UnloadContent() override {
        for (auto& screen : screens_) screen->UnloadContent();
    }

    void Update(GameTime& gameTime) override {
        screensToUpdate_.clear();
        for (auto& screen : screens_) screensToUpdate_.push_back(screen);

        bool otherScreenHasFocus = !getGameProperty().getIsActiveProperty();
        bool coveredByOtherScreen = false;

        while (!screensToUpdate_.empty()) {
            std::shared_ptr<GameScreen> screen = screensToUpdate_.back();
            screensToUpdate_.pop_back();

            screen->Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

            if (screen->GetScreenState() == ScreenState::TransitionOn ||
                screen->GetScreenState() == ScreenState::Active) {
                if (!otherScreenHasFocus) {
                    screen->HandleInput();
                    otherScreenHasFocus = true;
                }
                if (!screen->IsPopup()) coveredByOtherScreen = true;
            }
        }
    }

    void Draw(const GameTime& gameTime) override {
        for (auto& screen : screens_) {
            if (screen->GetScreenState() == ScreenState::Hidden) continue;
            screen->Draw(gameTime);
        }
    }

    void AddScreen(std::shared_ptr<GameScreen> screen) {
        screen->SetScreenManager(this);
        screen->SetIsExiting(false);
        if (isInitialized_) screen->LoadContent();
        screens_.push_back(std::move(screen));
    }

    void RemoveScreen(GameScreen* screen) {
        if (isInitialized_) screen->UnloadContent();
        EraseByPtr(screens_, screen);
        EraseByPtr(screensToUpdate_, screen);
    }

    std::vector<std::shared_ptr<GameScreen>> GetScreens() const { return screens_; }

private:
    static void EraseByPtr(std::vector<std::shared_ptr<GameScreen>>& v, GameScreen* p) {
        v.erase(std::remove_if(v.begin(), v.end(), [p](const std::shared_ptr<GameScreen>& s) { return s.get() == p; }),
                v.end());
    }

    std::vector<std::shared_ptr<GameScreen>> screens_;
    std::vector<std::shared_ptr<GameScreen>> screensToUpdate_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    bool isInitialized_ = false;
};

inline void GameScreen::RemoveFromScreenManager() { screenManager_->RemoveScreen(this); }

inline void GameScreen::ExitScreen() {
    SetIsExiting(true);
    if (TransitionOffTime().getTotalMillisecondsProperty() == 0.0) RemoveFromScreenManager();
}

} // namespace RolePlaying
