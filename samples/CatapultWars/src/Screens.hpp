#pragma once

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "System/Random.hpp"

#include "MenuScreen.hpp"
#include "Human.hpp"
#include "AI.hpp"
#include "AudioManager.hpp"

namespace CatapultWars {

using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

class MainMenuScreen;   // forward declarations for cross-referencing screens
class PauseScreen;
class GameplayScreen;
class InstructionsScreen;

// ===================== BackgroundScreen =====================
// Draws the fixed title-screen image behind the menus.
class BackgroundScreen : public GameScreen {
public:
    BackgroundScreen() {
        setTransitionOnTime(TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        background_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/title_screen"));
    }

    void Draw(const GameTime& /*gameTime*/) override {
        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
        spriteBatch.Begin();
        spriteBatch.Draw(*background_, Vector2(0, 0),
                         Color(255, 255, 255, TransitionAlphaByte()));
        spriteBatch.End();
    }

private:
    std::optional<Texture2D> background_;
};

// ===================== PauseScreen =====================
class PauseScreen : public MenuScreen {
public:
    PauseScreen(std::shared_ptr<GameScreen> backgroundScreen, Human* human, Player* computer)
        : MenuScreen(std::string()), backgroundScreen_(std::move(backgroundScreen)),
          human_(human), computer_(computer) {
        setIsPopup(true);

        auto returnEntry = std::make_shared<MenuEntry>("Return");
        auto quitEntry = std::make_shared<MenuEntry>("Quit Game");

        returnEntry->Selected = [this](PlayerIndex) { StartGameMenuEntrySelected(); };
        quitEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(returnEntry);
        MenuEntries().push_back(quitEntry);

        // Preserve and pause the game logic state.
        prevHumanIsActive_ = human_->getCatapult()->getIsActive();
        prevComputerIsActive_ = computer_->getCatapult()->getIsActive();

        human_->getCatapult()->setIsActive(false);
        computer_->getCatapult()->setIsActive(false);

        AudioManager::PauseResumeSounds(false);
    }

protected:
    void UpdateMenuEntryLocations() override {
        MenuScreen::UpdateMenuEntryLocations();
        for (auto& entry : MenuEntries()) {
            Vector2 position = entry->Position();
            position.Y += 60;
            entry->setPosition(position);
        }
    }

    // Defined out-of-line below (creates a MainMenuScreen).
    void OnCancel(PlayerIndex playerIndex) override;

private:
    void StartGameMenuEntrySelected() {
        human_->getCatapult()->setIsActive(prevHumanIsActive_);
        computer_->getCatapult()->setIsActive(prevComputerIsActive_);

        if (!human_->getIsDragging()) {
            AudioManager::PauseResumeSounds(true);
        } else {
            human_->ResetDragState();
            AudioManager::StopSounds();
        }

        backgroundScreen_->ExitScreen();
        ExitScreen();
    }

    std::shared_ptr<GameScreen> backgroundScreen_;
    Human* human_;
    Player* computer_;
    bool prevHumanIsActive_ = false;
    bool prevComputerIsActive_ = false;
};

// ===================== GameplayScreen =====================
class GameplayScreen : public GameScreen {
public:
    GameplayScreen() = default;

    void LoadContent() override {
        // Start the game (assets were pre-loaded by InstructionsScreen).
        Start();
    }

    // Loads all gameplay assets and creates the two players.
    void LoadAssets() {
        foregroundTexture_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/gameplay_screen"));
        cloud1Texture_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/cloud1"));
        cloud2Texture_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/cloud2"));
        mountainTexture_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/mountain"));
        skyTexture_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/sky"));
        defeatTexture_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/defeat"));
        victoryTexture_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/victory"));
        hudBackgroundTexture_.emplace(Content().Load<Texture2D>("Textures/HUD/hudBackground"));
        windArrowTexture_.emplace(Content().Load<Texture2D>("Textures/HUD/windArrow"));
        ammoTypeTexture_.emplace(Content().Load<Texture2D>("Textures/HUD/ammoType"));
        hudFont_.emplace(Content().Load<SpriteFont>("Fonts/HUDFont"));

        cloud1Position_ = Vector2(224 - cloud1Texture_->getWidthProperty(), 32);
        cloud2Position_ = Vector2(64, 90);

        playerHUDPosition_ = Vector2(7, 7);
        computerHUDPosition_ = Vector2(613, 7);
        windArrowPosition_ = Vector2(345, 46);

        Game& game = screenManager_->getGameProperty();
        SpriteBatch& sb = screenManager_->getSpriteBatch();

        player_ = std::make_shared<Human>(game, sb);
        player_->Initialize();
        player_->setName("Player");

        computer_ = std::make_shared<AI>(game, sb);
        computer_->Initialize();
        computer_->setName("Phone");

        player_->setEnemy(computer_.get());
        computer_->setEnemy(player_.get());
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        if ((player_->getCatapult()->getGameOver() || computer_->getCatapult()->getGameOver()) &&
            (gameOver_ == false)) {
            gameOver_ = true;
            if (player_->getScore() > computer_->getScore())
                AudioManager::PlaySound("gameOver_Win");
            else
                AudioManager::PlaySound("gameOver_Lose");
            return;
        }

        // New turn when a catapult finished its cycle (Reset, nothing animating).
        if ((player_->getCatapult()->getCurrentState() == CatapultState::Reset ||
             computer_->getCatapult()->getCurrentState() == CatapultState::Reset) &&
            !(player_->getCatapult()->getAnimationRunning() ||
              computer_->getCatapult()->getAnimationRunning())) {
            changeTurn_ = true;

            if (player_->getIsActive()) {  // last turn was the human turn?
                player_->setIsActive(false);
                computer_->setIsActive(true);
                isHumanTurn_ = false;
                player_->getCatapult()->setCurrentState(CatapultState::Idle);
                computer_->getCatapult()->setCurrentState(CatapultState::Aiming);
            } else {
                player_->setIsActive(true);
                computer_->setIsActive(false);
                isHumanTurn_ = true;
                computer_->getCatapult()->setCurrentState(CatapultState::Idle);
                player_->getCatapult()->setCurrentState(CatapultState::Idle);
            }
        }

        if (changeTurn_) {
            wind_ = Vector2((float)random_.Next(-1, 2), (float)random_.Next(minWind, maxWind + 1));
            float w = wind_.X > 0 ? wind_.Y : -wind_.Y;
            player_->getCatapult()->setWind(w);
            computer_->getCatapult()->setWind(w);
            changeTurn_ = false;
        }

        player_->Update(gameTime);
        computer_->Update(gameTime);

        UpdateClouds(elapsed);

        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
    }

    void Draw(const GameTime& gameTime) override {
        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
        spriteBatch.Begin();

        DrawBackground();
        DrawComputer(gameTime);
        DrawPlayer(gameTime);
        DrawHud();

        spriteBatch.End();
    }

    void HandleInput(InputState& input) override {
        if (gameOver_) {
            if (input.IsPauseGame(std::nullopt) || input.IsTap(std::nullopt))
                FinishCurrentGame();
            return;
        }

        if (input.IsPauseGame(std::nullopt)) {
            PauseCurrentGame();
        } else if (isHumanTurn_ &&
                   (player_->getCatapult()->getCurrentState() == CatapultState::Idle ||
                    player_->getCatapult()->getCurrentState() == CatapultState::Aiming)) {
            player_->HandleInput(input);
            isDragging_ = player_->getIsDragging();
        }
    }

private:
    void UpdateClouds(float elapsedTime) {
        int windDirection = wind_.X > 0 ? 1 : -1;
        int vpWidth = screenManager_->getGraphicsDeviceProperty().getViewportProperty().getWidthProperty();

        cloud1Position_ = cloud1Position_ +
            Vector2(24.0f, 0.0f) * elapsedTime * (float)windDirection * wind_.Y;
        if (cloud1Position_.X > vpWidth)
            cloud1Position_.X = -cloud1Texture_->getWidthProperty() * 2.0f;
        else if (cloud1Position_.X < -cloud1Texture_->getWidthProperty() * 2.0f)
            cloud1Position_.X = (float)vpWidth;

        cloud2Position_ = cloud2Position_ +
            Vector2(16.0f, 0.0f) * elapsedTime * (float)windDirection * wind_.Y;
        if (cloud2Position_.X > vpWidth)
            cloud2Position_.X = -cloud2Texture_->getWidthProperty() * 2.0f;
        else if (cloud2Position_.X < -cloud2Texture_->getWidthProperty() * 2.0f)
            cloud2Position_.X = (float)vpWidth;
    }

    void DrawPlayer(const GameTime& gameTime) {
        if (!gameOver_)
            player_->Draw(gameTime);
    }

    void DrawComputer(const GameTime& gameTime) {
        if (!gameOver_)
            computer_->Draw(gameTime);
    }

    void DrawBackground() {
        auto& device = screenManager_->getGraphicsDeviceProperty();
        SpriteBatch& sb = screenManager_->getSpriteBatch();

        device.Clear(Color::White);

        sb.Draw(*skyTexture_, Vector2::Zero, Color::White);
        sb.Draw(*cloud1Texture_, cloud1Position_, Color::White);
        sb.Draw(*mountainTexture_, Vector2::Zero, Color::White);
        sb.Draw(*cloud2Texture_, cloud2Position_, Color::White);
        sb.Draw(*foregroundTexture_, Vector2::Zero, Color::White);
    }

    void DrawHud() {
        SpriteBatch& sb = screenManager_->getSpriteBatch();
        auto& viewport = screenManager_->getGraphicsDeviceProperty().getViewportProperty();

        if (gameOver_) {
            const Texture2D& texture = (player_->getScore() > computer_->getScore())
                                           ? *victoryTexture_ : *defeatTexture_;
            sb.Draw(texture,
                    Vector2((float)(viewport.getWidthProperty() / 2 - texture.getWidthProperty() / 2),
                            (float)(viewport.getHeightProperty() / 2 - texture.getHeightProperty() / 2)),
                    Color::White);
            return;
        }

        // Player HUD
        sb.Draw(*hudBackgroundTexture_, playerHUDPosition_, Color::White);
        sb.Draw(*ammoTypeTexture_, playerHUDPosition_ + Vector2(33, 35), Color::White);
        DrawString(*hudFont_, std::to_string(player_->getScore()),
                   playerHUDPosition_ + Vector2(123, 35), Color::White);
        DrawString(*hudFont_, player_->getName(),
                   playerHUDPosition_ + Vector2(40, 1), Color::Blue);

        // Computer HUD
        sb.Draw(*hudBackgroundTexture_, computerHUDPosition_, Color::White);
        sb.Draw(*ammoTypeTexture_, computerHUDPosition_ + Vector2(33, 35), Color::White);
        DrawString(*hudFont_, std::to_string(computer_->getScore()),
                   computerHUDPosition_ + Vector2(123, 35), Color::White);
        DrawString(*hudFont_, computer_->getName(),
                   computerHUDPosition_ + Vector2(40, 1), Color::Red);

        // Wind direction
        std::string text = "WIND";
        Vector2 size = hudFont_->MeasureString(text);
        Vector2 windarrowScale(wind_.Y / 10, 1);
        sb.Draw(*windArrowTexture_, windArrowPosition_, std::nullopt, Color::White, 0.0f,
                Vector2::Zero, windarrowScale,
                wind_.X > 0 ? SpriteEffects::None : SpriteEffects::FlipHorizontally, 0.0f);

        DrawString(*hudFont_, text, windArrowPosition_ - Vector2(0, size.Y), Color::Black);
        if (wind_.Y == 0) {
            text = "NONE";
            DrawString(*hudFont_, text, windArrowPosition_, Color::Black);
        }

        if (isHumanTurn_) {
            text = !isDragging_ ? "Drag Anywhere to Fire" : "Release to Fire!";
            size = hudFont_->MeasureString(text);
        } else {
            text = "I'll get you yet!";
            size = hudFont_->MeasureString(text);
        }

        DrawString(*hudFont_, text,
                   Vector2(viewport.getWidthProperty() / 2 - size.X / 2,
                           viewport.getHeightProperty() - size.Y),
                   Color::Green);
    }

    // Draws shadowed text.
    void DrawString(SpriteFont& font, const std::string& text, Vector2 position, Color color) {
        SpriteBatch& sb = screenManager_->getSpriteBatch();
        sb.DrawString(font, text, Vector2(position.X + 1, position.Y + 1), Color::Black);
        sb.DrawString(font, text, position, color);
    }

    void FinishCurrentGame() {
        ExitScreen();
    }

    // Defined out-of-line below (creates BackgroundScreen + PauseScreen).
    void PauseCurrentGame();

    void Start() {
        wind_ = Vector2::Zero;
        isHumanTurn_ = false;
        changeTurn_ = true;
        computer_->getCatapult()->setCurrentState(CatapultState::Reset);
    }

    std::optional<Texture2D> foregroundTexture_, cloud1Texture_, cloud2Texture_, mountainTexture_,
                             skyTexture_, hudBackgroundTexture_, ammoTypeTexture_, windArrowTexture_,
                             defeatTexture_, victoryTexture_;
    std::optional<SpriteFont> hudFont_;

    Vector2 cloud1Position_, cloud2Position_, playerHUDPosition_, computerHUDPosition_, windArrowPosition_;

    std::shared_ptr<Human> player_;
    std::shared_ptr<AI> computer_;
    Vector2 wind_;
    bool changeTurn_ = false;
    bool isHumanTurn_ = false;
    bool gameOver_ = false;
    System::Random random_;
    static constexpr int minWind = 0;
    static constexpr int maxWind = 10;

    bool isDragging_ = false;
};

// ===================== InstructionsScreen =====================
class InstructionsScreen : public GameScreen {
public:
    InstructionsScreen() {
        setTransitionOnTime(TimeSpan::FromSeconds(0));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        background_.emplace(Content().Load<Texture2D>("Textures/Backgrounds/instructions"));
        font_.emplace(Content().Load<SpriteFont>("Fonts/MenuFont"));
    }

    void HandleInput(InputState& input) override {
        if (isLoading_)
            return;

        if (input.IsTap(std::nullopt)) {
            // Create the gameplay screen, pre-load its assets, then swap to it.
            // (The XNA original loaded on a background thread; we load inline.)
            auto gameplayScreen = std::make_shared<GameplayScreen>();
            gameplayScreen->setScreenManager(screenManager_);
            gameplayScreen->LoadAssets();
            isLoading_ = true;

            ExitScreen();
            screenManager_->AddScreen(gameplayScreen, std::nullopt);
        }
    }

    void Draw(const GameTime& /*gameTime*/) override {
        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
        spriteBatch.Begin();
        spriteBatch.Draw(*background_, Vector2(0, 0), Color(255, 255, 255, TransitionAlphaByte()));

        if (isLoading_) {
            std::string text = "Loading...";
            Vector2 size = font_->MeasureString(text);
            auto& vp = screenManager_->getGraphicsDeviceProperty().getViewportProperty();
            Vector2 position((vp.getWidthProperty() - size.X) / 2, (vp.getHeightProperty() - size.Y) / 2);
            spriteBatch.DrawString(*font_, text, position, Color::Black);
            spriteBatch.DrawString(*font_, text, position - Vector2(-4, 4), Color(255, 150, 0));
        }

        spriteBatch.End();
    }

private:
    std::optional<Texture2D> background_;
    std::optional<SpriteFont> font_;
    bool isLoading_ = false;
};

// ===================== MainMenuScreen =====================
class MainMenuScreen : public MenuScreen {
public:
    MainMenuScreen() : MenuScreen(std::string()) {
        setIsPopup(true);

        auto startGameMenuEntry = std::make_shared<MenuEntry>("Play");
        auto exitMenuEntry = std::make_shared<MenuEntry>("Exit");

        startGameMenuEntry->Selected = [this](PlayerIndex) {
            GetScreenManager()->AddScreen(std::make_shared<InstructionsScreen>(), std::nullopt);
        };
        exitMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(startGameMenuEntry);
        MenuEntries().push_back(exitMenuEntry);
    }

protected:
    void UpdateMenuEntryLocations() override {
        MenuScreen::UpdateMenuEntryLocations();
        for (auto& entry : MenuEntries()) {
            Vector2 position = entry->Position();
            position.Y += 60;
            entry->setPosition(position);
        }
    }

    void OnCancel(PlayerIndex /*playerIndex*/) override {
        GetScreenManager()->getGameProperty().Exit();
    }
};

// ---- cross-referencing method definitions (after all screens declared) ----

inline void PauseScreen::OnCancel(PlayerIndex /*playerIndex*/) {
    AudioManager::StopSounds();
    GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
    ExitScreen();
}

inline void GameplayScreen::PauseCurrentGame() {
    auto pauseMenuBackground = std::make_shared<BackgroundScreen>();

    if (isDragging_) {
        isDragging_ = false;
        player_->getCatapult()->setCurrentState(CatapultState::Idle);
    }

    screenManager_->AddScreen(pauseMenuBackground, std::nullopt);
    screenManager_->AddScreen(
        std::make_shared<PauseScreen>(pauseMenuBackground, player_.get(), computer_.get()),
        std::nullopt);
}

} // namespace CatapultWars
