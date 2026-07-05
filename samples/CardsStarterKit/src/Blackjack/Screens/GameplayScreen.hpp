#pragma once

// GameplayScreen.hpp -- C++ port of Screens/GameplayScreen.cs (XNA 4.0
// CardsStarterKit sample). Hosts a BlackjackCardGame instance and forwards
// input/update/draw to it; also implements pause (hide/disable gameplay
// components, show PauseScreen) and resume.

#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"

#include "../../GameStateManagement/GameScreen.hpp"
#include "../../GameStateManagement/ScreenManager.hpp"
#include "../BetGameComponent.hpp"
#include "../BlackjackCardGame.hpp"
#include "../BlackjackCommon.hpp"
#include "../InputHelper.hpp"
#include "BackgroundScreen.hpp"

namespace Blackjack {

using GameStateManagement::GameScreen;
using GameStateManagement::InputState;
using GameStateManagement::ScreenManager;
using Microsoft::Xna::Framework::DrawableGameComponent;

class PauseScreen;  // forward declaration (Screens/PauseScreen.hpp)

class GameplayScreen : public GameScreen {
public:
    explicit GameplayScreen(const std::string& theme) : theme_(theme) {
        setTransitionOnTime(TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        safeArea_ = GetScreenManager()->SafeArea();

        inputHelper_ = std::make_shared<InputHelper>(GetScreenManager()->getGameProperty());
        inputHelper_->setDrawOrderProperty(1000);
        GetScreenManager()->getGameProperty().getComponentsProperty().Add(inputHelper_.get());
        inputHelper_->setVisibleProperty(false);
        inputHelper_->setEnabledProperty(false);

        blackJackGame_ = std::make_shared<BlackjackCardGame>(
            GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty(),
            Vector2((float)(safeArea_.X + safeArea_.Width / 2 - 50), (float)(safeArea_.Y + 20)),
            [this](int player) { return GetPlayerCardPosition(player); }, *GetScreenManager(), theme_);

        InitializeGame();
    }

    void UnloadContent() override {
        (void)GetScreenManager()->getGameProperty().getComponentsProperty().Remove(inputHelper_.get());
    }

    void HandleInput(InputState& input) override {
        if (input.IsPauseGame(std::nullopt))
            PauseCurrentGame();
        GameScreen::HandleInput(input);
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        if (blackJackGame_ && !coveredByOtherScreen)
            blackJackGame_->Update(gameTime);

        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
    }

    void Draw(const GameTime& gameTime) override {
        GameScreen::Draw(gameTime);
        if (blackJackGame_)
            blackJackGame_->Draw(gameTime);
    }

    // Reveals and re-enables components hidden by PauseCurrentGame().
    void ReturnFromPause() {
        for (auto* component : pauseEnabledComponents_)
            component->setEnabledProperty(true);
        for (auto* component : pauseVisibleComponents_)
            component->setVisibleProperty(true);
    }

private:
    void InitializeGame() {
        blackJackGame_->Initialize();

        blackJackGame_->AddPlayer(std::make_shared<BlackjackPlayer>("Abe", blackJackGame_.get()));

        auto benny = std::make_shared<BlackjackAIPlayer>("Benny", blackJackGame_.get());
        blackJackGame_->AddPlayer(benny);
        benny->Hit.Add([this](System::Object*, const System::EventArgs&) { blackJackGame_->Hit(); });
        benny->Stand.Add([this](System::Object*, const System::EventArgs&) { blackJackGame_->Stand(); });

        auto chuck = std::make_shared<BlackjackAIPlayer>("Chuck", blackJackGame_.get());
        blackJackGame_->AddPlayer(chuck);
        chuck->Hit.Add([this](System::Object*, const System::EventArgs&) { blackJackGame_->Hit(); });
        chuck->Stand.Add([this](System::Object*, const System::EventArgs&) { blackJackGame_->Stand(); });

        // Note: the original requests "shuffle_" + theme (lowercase s), which
        // works on Windows' case-insensitive content pipeline even though the
        // shipped asset is "Shuffle_Red.png"/"Shuffle_Blue.png" (capital S).
        // CNA's Linux filesystem is case-sensitive, so this port requests the
        // asset's real casing instead -- see missing.md.
        std::string assets[] = {"blackjack", "bust", "lose", "push", "win", "pass", "Shuffle_" + theme_};
        for (auto& asset : assets)
            blackJackGame_->LoadUITexture("UI", asset);

        blackJackGame_->StartRound();
    }

    Vector2 GetPlayerCardPosition(int player) const {
        static const Vector2 kOffsets[] = {
            Vector2(100.0f, 190.0f),
            Vector2(336.0f, 210.0f),
            Vector2(570.0f, 190.0f),
        };
        if (player < 0 || player > 2)
            throw std::invalid_argument("Player index should be between 0 and 2");

        // Original: safeArea.Top + 200 * (BlackjackGame.HeightScale - 1); this
        // port fixes the backbuffer at 800x480 with no TV-safe-area scaling
        // (see missing.md), so HeightScale is always 1 and that term is 0.
        Rectangle safeArea = GetScreenManager()->SafeArea();
        return Vector2((float)safeArea.X, (float)safeArea.Y) + kOffsets[player];
    }

    void PauseCurrentGame();  // defined in PauseScreen.hpp (needs PauseScreen)

    std::shared_ptr<BlackjackCardGame> blackJackGame_;
    std::shared_ptr<InputHelper> inputHelper_;
    std::string theme_;
    std::vector<DrawableGameComponent*> pauseEnabledComponents_;
    std::vector<DrawableGameComponent*> pauseVisibleComponents_;
    Rectangle safeArea_;
};

} // namespace Blackjack
