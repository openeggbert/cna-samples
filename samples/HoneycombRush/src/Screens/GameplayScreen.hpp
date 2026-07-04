#pragma once

// GameplayScreen.hpp — C++ port of Screens/GameplayScreen.cs (XNA 4.0
// HoneycombRush sample). This is the largest file in the port: besides the
// GameplayScreen class itself, it also contains the out-of-line
// Update()/Draw()/Initialize()/HitBySmoke() bodies declared (but not
// defined) in Objects/*.hpp and Screens/{LoadingAndInstructionScreen,
// LevelOverScreen,PauseScreen}.hpp, since those all need GameplayScreen to be
// a complete type — see missing.md and the forward-declaration comments in
// each of those headers.
//
// Adaptation notes (see missing.md for full detail):
//  - Animations are loaded from a hand-translated static table instead of
//    parsing Content/Textures/AnimationsDefinition.xml at runtime (same
//    "no general XML deserializer" precedent as ConfigurationManager).
//  - Guide.BeginShowKeyboardInput has no CNA equivalent (Guide is a stub);
//    the high-score name-entry prompt is replaced with a fixed "Player" name.
//  - VirtualThumbsticks::getLeftPosition() replaces the original's own
//    lastTouchPosition field (see VirtualThumbsticks.hpp).

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/IGameComponent.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "System/TimeSpan.hpp"

#include "../Misc/Animation.hpp"
#include "../Misc/AudioManager.hpp"
#include "../Misc/ConfigurationManager.hpp"
#include "../Misc/ExtensionMethods.hpp"
#include "../Misc/VirtualThumbsticks.hpp"
#include "../Objects/Beehive.hpp"
#include "../Objects/BeeKeeper.hpp"
#include "../Objects/HoneyJar.hpp"
#include "../Objects/ScoreBar.hpp"
#include "../Objects/SoldierBee.hpp"
#include "../Objects/Vat.hpp"
#include "../Objects/WorkerBee.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"
#include "BackgroundScreen.hpp"
#include "HighScoreScreen.hpp"
#include "LevelOverScreen.hpp"
#include "LoadingAndInstructionScreen.hpp"
#include "PauseScreen.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::IGameComponent;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// The class that handles the entire game. Port of Screens/GameplayScreen.cs.
class GameplayScreen : public GameScreen {
public:
    explicit GameplayScreen(DifficultyMode gameDifficultyMode) {
        setTransitionOnTime(System::TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.0));
        startScreenTime_ = System::TimeSpan::FromSeconds(3);

        ConfigurationManager::CurrentDifficultyMode() = gameDifficultyMode;

        gameDifficultyLevel_ = gameDifficultyMode;
        gameElapsed_ = ConfigurationManager::ModesConfiguration().at(gameDifficultyLevel_).GameElapsed;

        amountOfSoldierBee_ = 4;
        amountOfWorkerBee_ = 16;

        controlstickBoundaryPosition_ = Vector2(34, 347);
        smokeButtonPosition_ = Vector2(664, 346);
        controlstickStartupPosition_ = Vector2(55, 369);

        SetIsInMotion(false);
        isAtStartupCountDown_ = true;
        isLevelEnd_ = false;

        setEnabledGestures(GestureType::Tap);
    }

    bool IsStarted() const { return !isAtStartupCountDown_ && !levelEnded_; }

    // Loads content and creates all game components. Called synchronously by
    // LoadingAndInstructionScreen/LevelOverScreen (see missing.md — the
    // original used a background System.Threading.Thread for this).
    void LoadAssets() {
        animations_.clear();
        LoadAnimationFromXML();
        LoadTextures();
        CreateGameComponents();

        AudioManager::PlayMusic("InGameSong_Loop");
    }

    void UnloadContent() override {
        auto& componentList = GetScreenManager()->getGameProperty().getComponentsProperty();
        IGameComponent* screenManagerComponent = static_cast<IGameComponent*>(GetScreenManager());

        for (int index = 0; index < (int)componentList.getCountProperty(); index++) {
            IGameComponent* component = componentList[index];
            if (component != screenManagerComponent && dynamic_cast<AudioManager*>(component) == nullptr) {
                componentList.RemoveAt(index);
                index--;
            }
        }

        GameScreen::UnloadContent();
    }

    void HandleInput(GameTime& gameTime, InputState& input) override {
        (void)gameTime;

        if (IsActive()) {
            VirtualThumbsticks::SetKeyboardAnchor(
                controlstickBoundaryPosition_ + Vector2((float)controlstickBoundary_.getWidthProperty() / 2.0f,
                                                         (float)controlstickBoundary_.getHeightProperty() / 2.0f));
            VirtualThumbsticks::Update(input);

            if (input.IsPauseGame(std::nullopt)) {
                PauseCurrentGame();
            }
        }

        isSmokebuttonClicked_ = false;

        PlayerIndex player;

        if (VirtualThumbsticks::getRightThumbstickCenter().has_value()) {
            Rectangle buttonRectangle((int)smokeButtonPosition_.X, (int)smokeButtonPosition_.Y,
                                       smokeButton_.getWidthProperty() / 2, smokeButton_.getHeightProperty());

            Vector2 rightCenter = *VirtualThumbsticks::getRightThumbstickCenter();
            Rectangle touchRectangle((int)rightCenter.X, (int)rightCenter.Y, 1, 1);

            if (buttonRectangle.Contains(touchRectangle) && !beeKeeper_->IsCollectingHoney && !beeKeeper_->IsStung()) {
                isSmokebuttonClicked_ = true;
            }
        }

        if (input.IsKeyDown(Keys::Space, ControllingPlayer(), player) && !beeKeeper_->IsCollectingHoney &&
            !beeKeeper_->IsStung()) {
            isSmokebuttonClicked_ = true;
        }

        if (!input.Gestures.empty()) {
            if (isLevelEnd_) {
                if (input.Gestures[0].getGestureTypeProperty() == GestureType::Tap) {
                    userTapToExit_ = true;
                }
            }
        }
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        if (isAtStartupCountDown_) {
            startScreenTime_ = startScreenTime_ - gameTime.getElapsedGameTimeProperty();
        }

        if (CheckIfCurrentGameFinished()) {
            GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
            return;
        }

        if (!(IsActive() && IsStarted())) {
            GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
            return;
        }

        gameElapsed_ = gameElapsed_ - gameTime.getElapsedGameTimeProperty();

        HandleThumbStick();
        HandleSmoke();

        beeKeeper_->SetDirection(VirtualThumbsticks::getLeftThumbstick());

        HandleCollision(gameTime);
        HandleVatHoneyArrow();

        beeKeeper_->setDrawOrderProperty(1);
        int beeKeeperY = (int)(beeKeeper_->Position().Y + (float)beeKeeper_->Bounds().Height - 2.0f);

        // Determine the beekeeper's draw order: if he is under half the
        // height of a beehive, he should be drawn over it.
        for (auto& beehive : beehives_) {
            if (beeKeeperY > beehive->Bounds().Y) {
                if (beehive->Bounds().Y + beehive->Bounds().Height / 2 < beeKeeperY) {
                    beeKeeper_->setDrawOrderProperty(
                        std::max((int)beeKeeper_->getDrawOrderProperty(), beehive->Bounds().Y + 1));
                }
            }
        }

        if (gameElapsed_.getMinutesProperty() == 0 && gameElapsed_.getSecondsProperty() == 10) {
            AudioManager::PlaySound("10SecondCountDown");
        }
        if (gameElapsed_.getMinutesProperty() == 0 && gameElapsed_.getSecondsProperty() == 30) {
            AudioManager::PlaySound("30SecondWarning");
        }

        vat_->DrawTimeLeft(gameElapsed_);

        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
    }

    void Draw(const GameTime& gameTime) override {
        SpriteBatch& sb = GetScreenManager()->getSpriteBatch();

        sb.Begin();

        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        Rectangle backgroundSrc(0, 0, background_.getWidthProperty(), background_.getHeightProperty());
        sb.Draw(background_, Rectangle(0, 0, viewport.getWidthProperty(), viewport.getHeightProperty()),
                backgroundSrc, Color::White, 0.0f, Vector2::Zero, SpriteEffects::None, 1.0f);

        if (isAtStartupCountDown_) {
            DrawStartupString();
        }

        if (IsActive() && IsStarted()) {
            DrawSmokeButton();

            sb.Draw(controlstickBoundary_, controlstickBoundaryPosition_, Color::White);
            sb.Draw(controlstick_, controlstickStartupPosition_, Color::White);

            sb.DrawString(*font16px_, SmokeText, Vector2(684, 456), Color::White);

            DrawVatHoneyArrow();
        }

        DrawLevelEndIfNecessary();

        sb.End();

        GameScreen::Draw(gameTime);
    }

private:
    static constexpr const char* SmokeText = "Smoke";

    // If the level is over, draws text describing the outcome.
    void DrawLevelEndIfNecessary() {
        if (isLevelEnd_) {
            std::string stringToDisplay;

            if (isUserWon_) {
                stringToDisplay = CheckIsInHighScore() ? "It's a new\nHigh-Score!" : "You Win!";
            } else {
                stringToDisplay = "Time Is Up!";
            }

            Vector2 stringVector = font36px_->MeasureString(stringToDisplay);
            auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();

            GetScreenManager()->getSpriteBatch().DrawString(
                *font36px_, stringToDisplay,
                Vector2((float)viewport.getWidthProperty() / 2.0f - stringVector.X / 2.0f,
                        (float)viewport.getHeightProperty() / 2.0f - stringVector.Y / 2.0f),
                Color::White);
        }
    }

    // Only reachable when at Hard difficulty.
    bool CheckIsInHighScore() const {
        return gameDifficultyLevel_ == DifficultyMode::Hard && HighScoreScreen::IsInHighscores(Score());
    }

    static std::string DifficultyModeToString(DifficultyMode mode) {
        switch (mode) {
            case DifficultyMode::Easy: return "Easy";
            case DifficultyMode::Medium: return "Medium";
            case DifficultyMode::Hard: return "Hard";
        }
        return "";
    }

    // Advances to the next screen based on the current difficulty and
    // whether or not the user has won.
    void MoveToNextScreen(bool isWon) {
        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("pauseBackground"), std::nullopt);

        if (isWon) {
            switch (gameDifficultyLevel_) {
                case DifficultyMode::Easy:
                case DifficultyMode::Medium: {
                    std::string text = "You Finished Level: " + DifficultyModeToString(gameDifficultyLevel_);
                    ++gameDifficultyLevel_;
                    GetScreenManager()->AddScreen(std::make_shared<LevelOverScreen>(text, gameDifficultyLevel_),
                                                   std::nullopt);
                    break;
                }
                case DifficultyMode::Hard:
                    GetScreenManager()->AddScreen(std::make_shared<LevelOverScreen>("You Win", std::nullopt),
                                                   std::nullopt);
                    break;
            }
        } else {
            GetScreenManager()->AddScreen(std::make_shared<LevelOverScreen>("You Lose", std::nullopt), std::nullopt);
        }

        AudioManager::StopMusic();
        AudioManager::StopSound("BeeBuzzing_Loop");
    }

    void PauseCurrentGame() {
        AudioManager::PauseResumeSounds(false);

        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("pauseBackground"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<PauseScreen>(), std::nullopt);
    }

    // Hand-translated from Content/Textures/AnimationsDefinition.xml — see
    // missing.md ("no general XML deserializer, hand-translate once").
    void LoadAnimationFromXML() {
        auto& content = GetScreenManager()->getGameProperty().getContentProperty();

        auto addAnimation = [&](const std::string& alias, const std::string& sheetName, int frameWidth,
                                 int frameHeight, int sheetColumns, int sheetRows) {
            Texture2D texture = content.Load<Texture2D>(sheetName);
            animations_[alias] = Animation(texture, Point(frameWidth, frameHeight), Point(sheetColumns, sheetRows));
        };

        addAnimation("WorkerBee", "Textures/beeWingFlap", 24, 24, 3, 1);
        addAnimation("SoldierBee", "Textures/SoldierBeeWingFlap", 24, 24, 3, 1);

        addAnimation("LegAnimation", "Textures/walkLegs", 85, 132, 16, 4);
        animations_["LegAnimation"].SetFrameInterval(System::TimeSpan::FromMilliseconds(75));

        addAnimation("ShootingAnimation", "Textures/shooting", 85, 132, 16, 4);
        addAnimation("BodyAnimation", "Textures/walkTorso", 85, 132, 16, 4);

        addAnimation("BeekeeperCollectingHoney", "Textures/collect", 85, 132, 4, 1);
        animations_["BeekeeperCollectingHoney"].SetSubAnimation(2, 3);
        animations_["BeekeeperCollectingHoney"].SetFrameInterval(System::TimeSpan::FromMilliseconds(150));

        addAnimation("BeekeeperDespositingHoney", "Textures/deposit", 85, 132, 4, 1);
        animations_["BeekeeperDespositingHoney"].SetFrameInterval(System::TimeSpan::FromMilliseconds(100));

        addAnimation("SmokeAnimation", "Textures/SmokeAnimationStrip", 85, 60, 4, 1);
    }

    void CreateGameComponents() {
        Game& game = GetScreenManager()->getGameProperty();

        auto jarScoreBar = std::make_shared<ScoreBar>(game, 0, 100, Vector2(8, 65), 10, 70, Color::Blue,
                                                       ScoreBar::ScoreBarOrientation::Horizontal, 0, this, true);
        game.getComponentsProperty().Add(jarScoreBar.get());
        ownedScoreBars_.push_back(jarScoreBar);

        jar_ = std::make_shared<HoneyJar>(game, this, Vector2(20, 8), jarScoreBar.get());
        game.getComponentsProperty().Add(jar_.get());

        CreateBeehives();

        int totalSmokeAmount = ConfigurationManager::ModesConfiguration().at(gameDifficultyLevel_).TotalSmokeAmount;
        Vector2 smokeButtonPos = Vector2(664, 346) + Vector2(22, (float)smokeButton_.getHeightProperty() - 8.0f);

        smokeButtonScorebar_ =
            std::make_shared<ScoreBar>(game, 0, totalSmokeAmount, smokeButtonPos, 12, 70, Color::White,
                                        ScoreBar::ScoreBarOrientation::Horizontal, totalSmokeAmount, this, false);
        game.getComponentsProperty().Add(smokeButtonScorebar_.get());

        beeKeeper_ = std::make_shared<BeeKeeper>(game, this);
        beeKeeper_->AnimationDefinitions = &animations_;
        beeKeeper_->ThumbStickArea =
            Rectangle((int)controlstickBoundaryPosition_.X, (int)controlstickBoundaryPosition_.Y,
                      controlstickBoundary_.getWidthProperty(), controlstickBoundary_.getHeightProperty());
        game.getComponentsProperty().Add(beeKeeper_.get());

        auto vatScoreBar = std::make_shared<ScoreBar>(game, 0, 300, Vector2(306, 440), 10, 190, Color::White,
                                                       ScoreBar::ScoreBarOrientation::Horizontal, 0, this, true);
        game.getComponentsProperty().Add(vatScoreBar.get());
        ownedScoreBars_.push_back(vatScoreBar);

        vat_ = std::make_shared<Vat>(game, this, game.getContentProperty().Load<Texture2D>("Textures/vat"),
                                      Vector2(294, 355), vatScoreBar.get());
        game.getComponentsProperty().Add(vat_.get());
        vatScoreBar->setDrawOrderProperty(vat_->getDrawOrderProperty() + 1);
    }

    void CreateBeehives() {
        Game& game = GetScreenManager()->getGameProperty();
        Vector2 scorebarPosition(18.0f, (float)beehiveTexture_.getHeightProperty() - 15.0f);
        Vector2 beehivePositions[5] = {
            Vector2(83, 8), Vector2(347, 8), Vector2(661, 8), Vector2(83, 201), Vector2(661, 201),
        };

        for (int i = 0; i < 5; i++) {
            auto scoreBar = std::make_shared<ScoreBar>(game, 0, 100, beehivePositions[i] + scorebarPosition, 10, 68,
                                                        Color::Green, ScoreBar::ScoreBarOrientation::Horizontal, 100,
                                                        this, false);
            game.getComponentsProperty().Add(scoreBar.get());
            ownedScoreBars_.push_back(scoreBar);

            auto beehive = std::make_shared<Beehive>(game, this, beehiveTexture_, scoreBar.get(), beehivePositions[i]);
            beehive->AnimationDefinitions = &animations_;
            game.getComponentsProperty().Add(beehive.get());
            beehives_.push_back(beehive);
            scoreBar->setDrawOrderProperty(beehive->getDrawOrderProperty());
        }

        for (int i = 0; i < 5; i++) {
            for (int s = 0; s < amountOfSoldierBee_; s++) {
                auto bee = std::make_shared<SoldierBee>(game, this, beehives_[i].get());
                bee->AnimationDefinitions = &animations_;
                game.getComponentsProperty().Add(bee.get());
                bees_.push_back(bee);
            }

            for (int w = 0; w < amountOfWorkerBee_; w++) {
                auto bee = std::make_shared<WorkerBee>(game, this, beehives_[i].get());
                bee->AnimationDefinitions = &animations_;
                game.getComponentsProperty().Add(bee.get());
                bees_.push_back(bee);
            }
        }
    }

    void LoadTextures() {
        auto& content = GetScreenManager()->getGameProperty().getContentProperty();

        beehiveTexture_ = content.Load<Texture2D>("Textures/beehive");
        background_ = content.Load<Texture2D>("Textures/Backgrounds/GamePlayBackground");
        controlstickBoundary_ = content.Load<Texture2D>("Textures/controlstickBoundary");
        controlstick_ = content.Load<Texture2D>("Textures/controlstick");
        smokeButton_ = content.Load<Texture2D>("Textures/smokeBtn");
        arrowTexture_ = content.Load<Texture2D>("Textures/arrow");
        font16px_.emplace(content.Load<SpriteFont>("Fonts/GameScreenFont16px"));
        font36px_.emplace(content.Load<SpriteFont>("Fonts/GameScreenFont36px"));
    }

    void HandleThumbStick() {
        Rectangle outerControlstick(0, (int)controlstickBoundaryPosition_.Y - 35,
                                     controlstickBoundary_.getWidthProperty() + 60,
                                     controlstickBoundary_.getHeightProperty() + 60);

        if (VirtualThumbsticks::getLeftThumbstick() == Vector2::Zero) {
            SetIsInMotion(false);
            beeKeeper_->SetMovement(Vector2::Zero);
            controlstickStartupPosition_ = Vector2(55, 369);
        } else {
            Vector2 lastTouch = VirtualThumbsticks::getLeftPosition();
            Rectangle touchRectangle((int)lastTouch.X, (int)lastTouch.Y, 1, 1);

            if (!outerControlstick.Contains(touchRectangle)) {
                controlstickStartupPosition_ = Vector2(55, 369);
                SetIsInMotion(false);
                return;
            }

            SetMotion();

            float radius = (float)controlstick_.getWidthProperty() / 2.0f + 35.0f;
            controlstickStartupPosition_ = Vector2(55, 369) + (VirtualThumbsticks::getLeftThumbstick() * radius);
        }
    }

    void SetMotion() {
        Vector2 leftThumbstick = VirtualThumbsticks::getLeftThumbstick();
        Vector2 tempVector = beeKeeper_->Position() + leftThumbstick * 12.0f;

        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();

        if (tempVector.X < 0 || tempVector.X + (float)beeKeeper_->Bounds().Width > (float)viewport.getWidthProperty())
            leftThumbstick.X = 0;
        if (tempVector.Y < 0 ||
            tempVector.Y + (float)beeKeeper_->Bounds().Height > (float)viewport.getHeightProperty())
            leftThumbstick.Y = 0;

        if (leftThumbstick == Vector2::Zero) {
            // Note: sets the field directly (not via SetIsInMotion()), so
            // beeKeeper_->IsInMotion is deliberately NOT updated here — this
            // matches a quirk in the original C# (which sets the private
            // backing field, bypassing its own IsInMotion property setter).
            isInMotion_ = false;
        } else {
            Vector2 beekeeperCalculatedPosition =
                Vector2((float)beeKeeper_->CentralCollisionArea().X, (float)beeKeeper_->CentralCollisionArea().Y) +
                leftThumbstick * 12.0f;

            if (!CheckBeehiveCollision(beekeeperCalculatedPosition)) {
                beeKeeper_->SetMovement(leftThumbstick * 12.0f);
                SetIsInMotion(true);
            }
        }
    }

    bool CheckBeehiveCollision(Vector2 beekeeperPosition) {
        Rectangle beekeeperTempCollisionArea((int)beekeeperPosition.X, (int)beekeeperPosition.Y,
                                              beeKeeper_->CentralCollisionArea().Width,
                                              beeKeeper_->CentralCollisionArea().Height);

        for (auto& beehive : beehives_) {
            if (beekeeperTempCollisionArea.Intersects(beehive->CentralCollisionArea())) {
                return true;
            }
        }
        return false;
    }

    void HandleCollision(GameTime& gameTime) {
        bool isCollectingHoney = HandleBeeKeeperBeehiveCollision();
        HandleSmokeBeehiveCollision();
        bool hasCollisionWithVat = HandleVatCollision();
        HandleBeeInteractions(gameTime, hasCollisionWithVat, isCollectingHoney);
    }

    void HandleBeeInteractions(GameTime& gameTime, bool isBeeKeeperCollideWithVat, bool isBeeKeeperCollideWithBeehive) {
        for (auto& bee : bees_) {
            SmokePuff* intersectingPuff = beeKeeper_->CheckSmokeCollision(bee->Bounds());
            if (intersectingPuff != nullptr) {
                bee->HitBySmoke(*intersectingPuff);
            }

            if (HasCollision(vat_->Bounds(), bee->Bounds())) {
                bee->Collide(vat_->Bounds());
            }

            if (HasCollision(beeKeeper_->Bounds(), bee->Bounds())) {
                if (!bee->IsBeeHit() && !isBeeKeeperCollideWithVat && !beeKeeper_->IsStung() &&
                    !beeKeeper_->IsFlashing() && !isBeeKeeperCollideWithBeehive) {
                    jar_->DecreaseHoneyByPercent(20);
                    beeKeeper_->Stung(gameTime.getTotalGameTimeProperty());
                    AudioManager::PlaySound("HoneyPotBreak");
                    AudioManager::PlaySound("Stung");
                }

                bee->Collide(beeKeeper_->Bounds());
            }

            if (auto* soldierBee = dynamic_cast<SoldierBee*>(bee.get())) {
                soldierBee->DistanceFromBeeKeeper =
                    Vector2::Distance(GetVector(beeKeeper_->Bounds()), GetVector(soldierBee->Bounds()));
                soldierBee->BeeKeeperVector = GetVector(beeKeeper_->Bounds()) - GetVector(soldierBee->Bounds());
            }
        }
    }

    bool HandleVatCollision() {
        if (HasCollision(beeKeeper_->Bounds(), vat_->VatDepositArea())) {
            if (jar_->HasHoney() && !beeKeeper_->IsStung() && !beeKeeper_->IsDepositingHoney() &&
                VirtualThumbsticks::getLeftThumbstick() == Vector2::Zero) {
                beeKeeper_->StartTransferHoney(4, [this]() { EndHoneyDeposit(); });
            }
            return true;
        }

        beeKeeper_->EndTransferHoney();
        return false;
    }

    void EndHoneyDeposit() {
        int honeyAmount = jar_->DecreaseHoneyByPercent(100);
        vat_->IncreaseHoney(honeyAmount);
        AudioManager::StopSound("DepositingIntoVat_Loop");
    }

    bool HandleBeeKeeperBeehiveCollision() {
        bool isCollidingWithBeehive = false;
        Beehive* collidedBeehive = nullptr;

        for (auto& beehive : beehives_) {
            if (HasCollision(beeKeeper_->Bounds(), beehive->Bounds())) {
                if (VirtualThumbsticks::getLeftThumbstick() == Vector2::Zero) {
                    collidedBeehive = beehive.get();
                    isCollidingWithBeehive = true;
                }
            } else {
                beehive->AllowBeesToGenerate = true;
            }
        }

        if (collidedBeehive != nullptr) {
            if (collidedBeehive->HasHoney() && jar_->CanCarryMore() && !beeKeeper_->IsStung()) {
                collidedBeehive->DecreaseHoney(1);
                jar_->IncreaseHoney(1);
                beeKeeper_->IsCollectingHoney = true;
                AudioManager::PlaySound("FillingHoneyPot_Loop");
            } else {
                beeKeeper_->IsCollectingHoney = false;
            }

            isCollidingWithBeehive = true;
            collidedBeehive->AllowBeesToGenerate = false;
        } else {
            beeKeeper_->IsCollectingHoney = false;
            AudioManager::StopSound("FillingHoneyPot_Loop");
        }

        return isCollidingWithBeehive;
    }

    void HandleSmokeBeehiveCollision() {
        for (auto& beehive : beehives_) {
            for (auto& smokePuff : beeKeeper_->FiredSmokePuffs()) {
                if (HasCollision(beehive->Bounds(), smokePuff->CentralCollisionArea())) {
                    beehive->AllowBeesToGenerate = false;
                }
            }
        }
    }

    void HandleVatHoneyArrow() { drawArrow_ = jar_->HasHoney(); }

    void HandleSmoke() {
        auto& config = ConfigurationManager::ModesConfiguration().at(gameDifficultyLevel_);

        if (!isSmokebuttonClicked_) {
            smokeButtonScorebar_->IncreaseCurrentValue(config.IncreaseAmountSpeed);
            beeKeeper_->setIsShootingSmoke(false);
        } else {
            if (smokeButtonScorebar_->CurrentValue() <= smokeButtonScorebar_->MinValue) {
                beeKeeper_->setIsShootingSmoke(false);
            } else {
                beeKeeper_->setIsShootingSmoke(true);
                smokeButtonScorebar_->DecreaseCurrentValue(config.DecreaseAmountSpeed);
            }
        }
    }

    // Checks whether the current game is over, and if so performs the
    // necessary actions. Always returns false (matching the original, which
    // never actually uses the early-return path this feeds in Update()).
    bool CheckIfCurrentGameFinished() {
        levelEnded_ = false;
        isUserWon_ = vat_->CurrentVatCapacity() >= vat_->MaxVatCapacity();

        if (isUserWon_ || gameElapsed_ <= System::TimeSpan::Zero) {
            levelEnded_ = true;
        }

        if (gameElapsed_ <= System::TimeSpan::Zero || levelEnded_) {
            isLevelEnd_ = true;
            if (userTapToExit_) {
                GetScreenManager()->RemoveScreen(this);

                if (isUserWon_) {
                    // CNA has no Guide.BeginShowKeyboardInput (Guide is a
                    // stub) — use a fixed placeholder name instead of an
                    // interactive name-entry prompt (see missing.md).
                    if (CheckIsInHighScore()) {
                        HighScoreScreen::PutHighScore("Player", Score());
                    }

                    AudioManager::PlaySound("Victory");
                } else {
                    AudioManager::PlaySound("Defeat");
                }

                MoveToNextScreen(isUserWon_);
            }
        }

        return false;
    }

    // Draws the arrow above the vat in blinking intervals of 20 update loops.
    void DrawVatHoneyArrow() {
        if (drawArrow_ && drawArrowInterval_) {
            GetScreenManager()->getSpriteBatch().Draw(arrowTexture_, Vector2(370, 314), Color::White);
            if (arrowCounter_ == 20) {
                drawArrowInterval_ = false;
                arrowCounter_ = 0;
            }
            arrowCounter_++;
        } else {
            if (arrowCounter_ == 20) {
                drawArrowInterval_ = true;
                arrowCounter_ = 0;
            }
            arrowCounter_++;
        }
    }

    void DrawSmokeButton() {
        Rectangle dest((int)smokeButtonPosition_.X, (int)smokeButtonPosition_.Y, 109, 109);
        Rectangle src = isSmokebuttonClicked_ ? Rectangle(109, 0, 109, 109) : Rectangle(0, 0, 109, 109);
        GetScreenManager()->getSpriteBatch().Draw(smokeButton_, dest, src, Color::White);
    }

    // Draws the pre-game countdown string. Note: measures with font16px_ but
    // draws with font36px_ — a faithfully-reproduced quirk of the original.
    void DrawStartupString() {
        if (isAtStartupCountDown_) {
            std::string text;

            if (startScreenTime_.getSecondsProperty() == 0) {
                text = "Go!";
                isAtStartupCountDown_ = false;
                AudioManager::PlaySound("BeeBuzzing_Loop", true, 0.6f);
            } else {
                text = std::to_string(startScreenTime_.getSecondsProperty());
            }

            Vector2 size = font16px_->MeasureString(text);
            auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
            Vector2 textPosition =
                (Vector2((float)viewport.getWidthProperty(), (float)viewport.getHeightProperty()) - size) / 2.0f;

            GetScreenManager()->getSpriteBatch().DrawString(*font36px_, text, textPosition, Color::White);
        }
    }

    void SetIsInMotion(bool value) {
        isInMotion_ = value;
        if (beeKeeper_) {
            beeKeeper_->IsInMotion = isInMotion_;
        }
    }

    int Score() const {
        int highscoreFactor = ConfigurationManager::ModesConfiguration().at(gameDifficultyLevel_).HighScoreFactor;
        return highscoreFactor * (int)gameElapsed_.getTotalMillisecondsProperty();
    }

    std::optional<SpriteFont> font16px_;
    std::optional<SpriteFont> font36px_;

    Texture2D arrowTexture_;
    Texture2D background_;
    Texture2D controlstickBoundary_;
    Texture2D controlstick_;
    Texture2D beehiveTexture_;
    Texture2D smokeButton_;
    std::shared_ptr<ScoreBar> smokeButtonScorebar_;

    Vector2 controlstickStartupPosition_;
    Vector2 controlstickBoundaryPosition_;
    Vector2 smokeButtonPosition_;

    bool isSmokebuttonClicked_ = false;
    bool drawArrow_ = false;
    bool drawArrowInterval_ = false;
    bool isInMotion_ = false;
    bool isAtStartupCountDown_ = false;
    bool isLevelEnd_ = false;
    bool levelEnded_ = false;
    bool isUserWon_ = false;
    bool userTapToExit_ = false;

    std::unordered_map<std::string, Animation> animations_;

    int amountOfSoldierBee_ = 0;
    int amountOfWorkerBee_ = 0;
    int arrowCounter_ = 0;

    std::vector<std::shared_ptr<Beehive>> beehives_;
    std::vector<std::shared_ptr<Bee>> bees_;
    std::vector<std::shared_ptr<ScoreBar>> ownedScoreBars_;

    System::TimeSpan gameElapsed_;
    System::TimeSpan startScreenTime_;

    std::shared_ptr<BeeKeeper> beeKeeper_;
    std::shared_ptr<HoneyJar> jar_;
    std::shared_ptr<Vat> vat_;

    DifficultyMode gameDifficultyLevel_ = DifficultyMode::Easy;
};

// ---- Objects/*.hpp and Screens/*.hpp methods that depend on GameplayScreen
// (defined here, since they need it to be a complete type) ----

inline void ScoreBar::Draw(const GameTime& gameTime) {
    if (!gameplayScreen_->IsActive()) {
        DrawableGameComponent::Draw(gameTime);
        return;
    }

    float rotation = (orientation_ == ScoreBarOrientation::Horizontal) ? 0.0f : 1.57f;

    SpriteBatch& sb = *spriteBatch_;
    sb.Begin();

    Rectangle backgroundSrc(0, 0, backgroundTexture_->getWidthProperty(), backgroundTexture_->getHeightProperty());
    sb.Draw(*backgroundTexture_, Rectangle((int)Position.X, (int)Position.Y, width_, height_), backgroundSrc,
            BarColor, rotation, Vector2::Zero, SpriteEffects::None, 0.0f);

    float spaceFromBorder = GetSpaceFromBorder() + 4.0f;
    Texture2D& coloredTexture = GetTextureByCurrentValue();
    Rectangle coloredSrc(0, 0, coloredTexture.getWidthProperty(), coloredTexture.getHeightProperty());

    if (orientation_ == ScoreBarOrientation::Horizontal) {
        sb.Draw(coloredTexture,
                Rectangle((int)Position.X + 2, (int)Position.Y + 2, width_ - (int)spaceFromBorder, height_ - 4),
                coloredSrc, Color::White, rotation, Vector2::Zero, SpriteEffects::None, 0.0f);
    } else {
        sb.Draw(coloredTexture,
                Rectangle((int)Position.X + 2 - height_, (int)Position.Y + width_ - 2,
                          width_ - (int)spaceFromBorder, height_ - 4),
                coloredSrc, BarColor, -rotation, Vector2::Zero, SpriteEffects::None, 0.0f);
    }

    sb.End();

    DrawableGameComponent::Draw(gameTime);
}

inline void Beehive::Update(GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Update(gameTime);
        return;
    }

    if (LastTimeHoneyAdded() == System::TimeSpan::Zero) {
        LastTimeHoneyAdded() = gameTime.getTotalGameTimeProperty();
        ScoreBarRef().IncreaseCurrentValue(1);
    } else if (LastTimeHoneyAdded() + IntervalToAddHoney() < gameTime.getTotalGameTimeProperty()) {
        LastTimeHoneyAdded() = gameTime.getTotalGameTimeProperty();
        ScoreBarRef().IncreaseCurrentValue(1);
    }

    DrawableGameComponent::Update(gameTime);
}

inline void Beehive::Draw(const GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Draw(gameTime);
        return;
    }

    spriteBatch->Begin();
    spriteBatch->Draw(texture, position, Color::White);
    spriteBatch->End();

    DrawableGameComponent::Draw(gameTime);
}

inline void HoneyJar::Draw(const GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Draw(gameTime);
        return;
    }

    spriteBatch->Begin();
    spriteBatch->Draw(texture, position, Color::White);
    spriteBatch->DrawString(*font16px_, HoneyText, position + Vector2(-12, (float)texture.getHeightProperty() + 10),
                             Color::White);
    spriteBatch->End();

    DrawableGameComponent::Draw(gameTime);
}

inline void Vat::Draw(const GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Draw(gameTime);
        return;
    }

    spriteBatch->Begin();
    spriteBatch->Draw(texture, position, Color::White);

    spriteBatch->DrawString(
        *font16px_, TimeLeftString,
        position + Vector2((float)texture.getWidthProperty() / 2.0f - timeleftStringSize_.X / 2.0f,
                            timeleftStringSize_.Y - 8.0f),
        Color::White, 0.0f, Vector2::Zero, 0.0f, SpriteEffects::None, 2.0f);

    Vector2 timeDigStringSize = font36px_->MeasureString(timeLeftString_);
    Color colorToDraw = Color::White;
    if (timeLeft_.getMinutesProperty() == 0 &&
        (timeLeft_.getSecondsProperty() == 30 || timeLeft_.getSecondsProperty() <= 10)) {
        colorToDraw = Color::Red;
    }

    spriteBatch->DrawString(*font36px_, timeLeftString_,
                             position + Vector2((float)texture.getWidthProperty() / 2.0f - timeDigStringSize.X / 2.0f,
                                                 timeDigStringSize.Y - 30.0f),
                             colorToDraw);

    spriteBatch->DrawString(
        *font14px_, EmptyString,
        Vector2(position.X, position.Y + (float)texture.getHeightProperty() - emptyStringSize_.Y), Color::White);

    spriteBatch->DrawString(*font14px_, FullString,
                             Vector2(position.X + (float)texture.getWidthProperty() - fullStringSize_.X,
                                     position.Y + (float)texture.getHeightProperty() - emptyStringSize_.Y),
                             Color::White);

    spriteBatch->End();

    DrawableGameComponent::Draw(gameTime);
}

inline void SmokePuff::Update(GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Update(gameTime);
        return;
    }

    lifeTime_ = lifeTime_ - gameTime.getElapsedGameTimeProperty();

    if (lifeTime_ <= System::TimeSpan::Zero) {
        (void)getGameProperty().getComponentsProperty().Remove(this);
        isInGameComponents_ = false;
        DrawableGameComponent::Update(gameTime);
        return;
    }

    growthTimeTrack_ = growthTimeTrack_ + gameTime.getElapsedGameTimeProperty();

    if (spreadFactor_ < 1.0f && growthTimeTrack_ >= GrowthTimeInterval()) {
        growthTimeTrack_ = System::TimeSpan::Zero;
        spreadFactor_ += GrowthStep;
    }

    if (Vector2::Dot(initialVelocity_, velocity_) > 0) {
        position = position + velocity_;
        velocity_ = velocity_ + acceleration_ * (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
    }

    DrawableGameComponent::Update(gameTime);
}

inline void SmokePuff::Draw(const GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Draw(gameTime);
        return;
    }

    spriteBatch->Begin();

    Vector2 offset = GetRandomOffset();

    spriteBatch->Draw(texture, position + offset, std::nullopt, Color::White, 0.0f, drawOrigin_, spreadFactor_,
                       SpriteEffects::None, 0.0f);

    spriteBatch->End();

    DrawableGameComponent::Draw(gameTime);
}

inline void Bee::Update(GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Update(gameTime);
        return;
    }

    if (!HandleRegeneration(gameTime)) {
        return;
    }

    if (!animationKey_.empty()) {
        (*AnimationDefinitions)[animationKey_].Update(gameTime, true);
    }

    if (!isHitBySmoke_) {
        SetRandomMovement();
    }

    position = position + velocity_;

    if (isHitBySmoke_) {
        position = position + velocity_;
    }

    auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
    int texThird = texture.getWidthProperty() / 3;

    if (position.X < 0 || position.X > (float)(viewport.getWidthProperty() - texThird) || position.Y < 0 ||
        position.Y > (float)(viewport.getHeightProperty() - texture.getHeightProperty())) {
        if (isHitBySmoke_) {
            SetStartupPositionWithTimer();
        } else {
            velocityChangeCounter_ = -5;
            if (position.X < (float)texThird || position.X > (float)(viewport.getWidthProperty() - texThird)) {
                velocity_.X *= -1.0f;
            } else {
                velocity_.Y *= -1.0f;
            }
        }
    }

    DrawableGameComponent::Update(gameTime);
}

inline void Bee::Draw(const GameTime& gameTime) {
    if (gamePlayScreen->IsActive()) {
        spriteBatch->Begin();

        if (!animationKey_.empty()) {
            (*AnimationDefinitions)[animationKey_].Draw(*spriteBatch, position, SpriteEffects::None);
        } else {
            spriteBatch->Draw(texture, position, std::nullopt, Color::White, 0.0f, Vector2::Zero, 1.0f,
                               SpriteEffects::None, 0.0f);
        }

        spriteBatch->End();
    }

    DrawableGameComponent::Draw(gameTime);
}

inline void Bee::HitBySmoke(SmokePuff& smokePuff) {
    if (!isHitBySmoke_) {
        Vector2 escapeVector =
            GetVector(Bounds().getCenterProperty()) - GetVector(smokePuff.Bounds().getCenterProperty());
        escapeVector.Normalize();
        escapeVector = escapeVector * (float)random_.Next(3, 6);

        velocity_ = escapeVector;

        isHitBySmoke_ = true;
    }
}

inline void SoldierBee::Update(GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        Bee::Update(gameTime);
        return;
    }

    if (isHitBySmoke_) {
        Bee::Update(gameTime);
        isChaseMode_ = false;
    } else {
        if (isChaseMode_) {
            velocity_ = BeeKeeperVector / AccelerationFactor();
            position = position + velocity_;
            (*AnimationDefinitions)[animationKey_].Update(gameTime, true);

            if (DistanceFromBeeKeeper <= 10.0f) {
                isChaseMode_ = false;
                SetStartupPosition();
            }
        } else {
            if (DistanceFromBeeKeeper != 0.0f && DistanceFromBeeKeeper <= chaseDistance_) {
                isChaseMode_ = true;
            } else {
                Bee::Update(gameTime);
            }
        }
    }
}

inline void BeeKeeper::Initialize() {
    (*AnimationDefinitions)[LegAnimationKey].PlayFromFrameIndex(0);
    (*AnimationDefinitions)[BodyAnimationKey].PlayFromFrameIndex(0);
    (*AnimationDefinitions)[SmokeAnimationKey].PlayFromFrameIndex(0);
    (*AnimationDefinitions)[ShootingAnimationKey].PlayFromFrameIndex(0);
    (*AnimationDefinitions)[BeekeeperCollectingHoneyAnimationKey].PlayFromFrameIndex(0);
    (*AnimationDefinitions)[BeekeeperDespositingHoneyAnimationKey].PlayFromFrameIndex(0);

    isStung_ = false;

    DrawableGameComponent::Initialize();
}

inline void BeeKeeper::Update(GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Update(gameTime);
        return;
    }

    if (IsCollectingHoney) {
        if (collectingHoneyFrameCounter_ > 3) {
            (*AnimationDefinitions)[BeekeeperCollectingHoneyAnimationKey].Update(gameTime, true, true);
        } else {
            (*AnimationDefinitions)[BeekeeperCollectingHoneyAnimationKey].Update(gameTime, true, false);
        }

        collectingHoneyFrameCounter_++;
    } else {
        collectingHoneyFrameCounter_ = 0;
    }

    if (isDepositingHoney_) {
        if (depositHoneyUpdatingTimer_ == System::TimeSpan::Zero) {
            depositHoneyUpdatingTimer_ = gameTime.getTotalGameTimeProperty();
        }

        (*AnimationDefinitions)[BeekeeperDespositingHoneyAnimationKey].Update(gameTime, true);
    }

    // The oldest smoke puff might have expired and should be recycled.
    if (!firedSmokePuffs_.empty() && firedSmokePuffs_.front()->IsGone()) {
        availableSmokePuffs_.push(firedSmokePuffs_.front());
        firedSmokePuffs_.pop_front();
    }

    if (isStung_ || isFlashing_) {
        stungDrawingCounter_++;

        if (stungDrawingCounter_ > stungDrawingInterval_) {
            stungDrawingCounter_ = 0;
            isDrawnLastStungInterval_ = !isDrawnLastStungInterval_;
        }

        if (stungTime_ + stungDuration_ < gameTime.getTotalGameTimeProperty()) {
            isStung_ = false;

            if (stungTime_ + stungDuration_ + flashingDuration_ < gameTime.getTotalGameTimeProperty()) {
                isFlashing_ = false;
                stungDrawingCounter_ = -1;
            }

            (*AnimationDefinitions)[LegAnimationKey].Update(gameTime, IsInMotion);
        }
    } else {
        (*AnimationDefinitions)[LegAnimationKey].Update(gameTime, IsInMotion);
    }

    if (needToShootSmoke_) {
        (*AnimationDefinitions)[SmokeAnimationKey].Update(gameTime, needToShootSmoke_);

        shootSmokePuffTimer_ = shootSmokePuffTimer_ - gameTime.getElapsedGameTimeProperty();
        if (shootSmokePuffTimer_ <= System::TimeSpan::Zero) {
            ShootSmoke();
            shootSmokePuffTimer_ = shootSmokePuffTimerInitialValue_;
        }
    }

    DrawableGameComponent::Update(gameTime);
}

inline void BeeKeeper::Draw(const GameTime& gameTime) {
    if (!gamePlayScreen->IsActive()) {
        DrawableGameComponent::Draw(gameTime);
        return;
    }

    if (isStung_ || isFlashing_) {
        if (stungDrawingCounter_ != stungDrawingInterval_) {
            if (isDrawnLastStungInterval_) {
                return;
            }
        }
    }

    spriteBatch->Begin();

    if (isStung_) {
        spriteBatch->Draw(hitTexture_, position, Color::White);
        spriteBatch->End();
        return;
    }

    if (IsCollectingHoney) {
        (*AnimationDefinitions)[BeekeeperCollectingHoneyAnimationKey].Draw(*spriteBatch, position,
                                                                            SpriteEffects::None);
        spriteBatch->End();
        return;
    }

    if (isDepositingHoney_) {
        if (VirtualThumbsticks::getLeftThumbstick() != Vector2::Zero) {
            isDepositingHoney_ = false;
            AudioManager::StopSound("DepositingIntoVat_Loop");
        }

        if (depositHoneyUpdatingTimer_ != System::TimeSpan::Zero &&
            depositHoneyUpdatingTimer_ + depositHoneyUpdatingInterval_ < gameTime.getTotalGameTimeProperty()) {
            depositHoneyTimerCounter_++;
            depositHoneyUpdatingTimer_ = System::TimeSpan::Zero;
        }

        (*AnimationDefinitions)[BeekeeperDespositingHoneyAnimationKey].Draw(*spriteBatch, position,
                                                                             SpriteEffects::None);

        if (depositHoneyTimerCounter_ == honeyDepositFrameCount_ - 1) {
            isDepositingHoney_ = false;
            if (depositHoneyCallback_) {
                depositHoneyCallback_();
            }
            (*AnimationDefinitions)[BeekeeperDespositingHoneyAnimationKey].PlayFromFrameIndex(0);
        }

        spriteBatch->End();
        return;
    }

    bool hadDirectionChanged = false;
    WalkingDirection tempDirection = direction_;

    DetermineDirection(tempDirection, smokeAdjustment_);

    if (tempDirection != direction_) {
        hadDirectionChanged = true;
        direction_ = tempDirection;
    }

    if (hadDirectionChanged) {
        lastFrameCounter_ = 0;
        (*AnimationDefinitions)[LegAnimationKey].PlayFromFrameIndex(lastFrameCounter_ + (int)direction_);
        (*AnimationDefinitions)[ShootingAnimationKey].PlayFromFrameIndex(lastFrameCounter_ + (int)direction_);
        (*AnimationDefinitions)[BodyAnimationKey].PlayFromFrameIndex(lastFrameCounter_ + (int)direction_);
    } else {
        if (lastFrameCounter_ == 8) {
            lastFrameCounter_ = 0;
            (*AnimationDefinitions)[LegAnimationKey].PlayFromFrameIndex(lastFrameCounter_ + (int)direction_);
            (*AnimationDefinitions)[ShootingAnimationKey].PlayFromFrameIndex(lastFrameCounter_ + (int)direction_);
            (*AnimationDefinitions)[BodyAnimationKey].PlayFromFrameIndex(lastFrameCounter_ + (int)direction_);
        } else {
            lastFrameCounter_++;
        }
    }

    (*AnimationDefinitions)[LegAnimationKey].Draw(*spriteBatch, position, 1.0f, SpriteEffects::None);

    if (needToShootSmoke_) {
        (*AnimationDefinitions)[ShootingAnimationKey].Draw(*spriteBatch, position, 1.0f, SpriteEffects::None);

        if (smokeAdjustment_ != Vector2::Zero) {
            (*AnimationDefinitions)[SmokeAnimationKey].Draw(*spriteBatch, position + smokeAdjustment_, 1.0f,
                                                             GetSpriteEffect(VirtualThumbsticks::getLeftThumbstick()));
        }
    } else {
        (*AnimationDefinitions)[BodyAnimationKey].Draw(*spriteBatch, position, 1.0f, SpriteEffects::None);
    }

    spriteBatch->End();

    DrawableGameComponent::Draw(gameTime);
}

inline void PauseScreen::ReturnGameMenuEntrySelected(PlayerIndex playerIndex) {
    (void)playerIndex;

    AudioManager::PauseResumeSounds(true);

    for (auto& screen : GetScreenManager()->GetScreens()) {
        if (dynamic_cast<GameplayScreen*>(screen.get()) == nullptr) {
            screen->ExitScreen();
        }
    }
}

inline void LoadingAndInstructionScreen::LoadContent() {
    font_.emplace(Load<SpriteFont>("Fonts/MenuFont"));

    gameplayScreen_ = std::make_shared<GameplayScreen>(DifficultyMode::Easy);
    gameplayScreen_->setScreenManager(GetScreenManager());
}

inline void LoadingAndInstructionScreen::Update(GameTime& gameTime, bool otherScreenHasFocus,
                                                 bool coveredByOtherScreen) {
    if (isLoading_) {
        if (loadPending_) {
            if (!IsExiting()) {
                gameplayScreen_->LoadAssets();

                for (auto& screen : GetScreenManager()->GetScreens())
                    screen->ExitScreen();

                GetScreenManager()->AddScreen(gameplayScreen_, std::nullopt);
            }
            loadPending_ = false;
            isLoading_ = false;
        } else {
            loadPending_ = true;
        }
    }

    GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
}

inline void LevelOverScreen::LoadContent() {
    if (difficultyMode_.has_value()) {
        gameplayScreen_ = std::make_shared<GameplayScreen>(*difficultyMode_);
        gameplayScreen_->setScreenManager(GetScreenManager());
    }

    auto& content = GetScreenManager()->getGameProperty().getContentProperty();
    font36px_.emplace(content.Load<SpriteFont>("Fonts/GameScreenFont36px"));
    font16px_.emplace(content.Load<SpriteFont>("Fonts/GameScreenFont16px"));
    textSize_ = font36px_->MeasureString(text_);
}

inline void LevelOverScreen::Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) {
    if (isLoading_) {
        if (loadPending_) {
            gameplayScreen_->LoadAssets();

            for (auto& screen : GetScreenManager()->GetScreens())
                screen->ExitScreen();

            GetScreenManager()->AddScreen(gameplayScreen_, std::nullopt);

            loadPending_ = false;
            isLoading_ = false;
        } else {
            loadPending_ = true;
        }
    }

    GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
}

inline void LevelOverScreen::StartNewLevelOrExit(InputState& input) {
    (void)input;

    if (!difficultyMode_.has_value()) {
        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("highScoreScreen"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<HighScoreScreen>(), std::nullopt);
    } else if (!isLoading_) {
        isLoading_ = true;
    }
}

} // namespace HoneycombRush
