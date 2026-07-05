#pragma once

// GameplayScreen.hpp — C++ port of Screens/GameplayScreen.cs (XNA 4.0
// NinjAcademy sample). This is the largest file in the port: besides the
// GameplayScreen class itself, it also contains the out-of-line
// LoadContent()/Update()/StartSelected()/HighScoreSelected() bodies of
// MainMenuScreen, LoadContent()/Update() of LoadingScreen,
// ResumeSelected() of PauseScreen, and Update() of CountdownScreen -- all
// declared (but not defined) in their own headers since they need
// GameplayScreen to be a complete type. See missing.md for the reasoning
// behind this file-splitting technique (also used by HoneycombRush).
//
// Adaptation notes (see missing.md for full detail):
//  - Windows Phone tombstoning (PhoneApplicationService state save/restore)
//    is dropped; the game always starts a fresh session.
//  - Guide.BeginShowKeyboardInput has no CNA equivalent that can return real
//    text; replaced with NameEntryScreen (see Screens/HighScoreScreen.hpp),
//    a small keyboard-driven popup.
//  - Background-thread asset loading is simplified to a synchronous call.

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/Random.hpp"
#include "System/TimeSpan.hpp"

#include "../AnimationStore.hpp"
#include "../AudioManager.hpp"
#include "../GameConfiguration.hpp"
#include "../GameConstants.hpp"
#include "../Line.hpp"
#include "../Elements/HUD/HitPointsComponent.hpp"
#include "../Elements/HUD/ScoreComponent.hpp"
#include "../Elements/HUD/TextDisplayComponent.hpp"
#include "../Elements/DisappearingAnimationComponent.hpp"
#include "../Elements/LaunchedComponent.hpp"
#include "../Elements/Specific/SwordSlash.hpp"
#include "../Elements/Specific/Target.hpp"
#include "../Elements/Specific/ThrowingStar.hpp"
#include "../Elements/StaticTextureComponent.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "BackgroundScreen.hpp"
#include "CountdownScreen.hpp"
#include "HighScoreScreen.hpp"
#include "LoadingScreen.hpp"
#include "MainMenuScreen.hpp"
#include "PauseScreen.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::BoundingBox;
using Microsoft::Xna::Framework::ContainmentType;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Port of Screens/GameplayScreen.cs.
class GameplayScreen : public GameScreen {
public:
    GameplayScreen() { setEnabledGestures(GestureType::Tap | GestureType::FreeDrag); }

    int Score() const { return scoreComponent_->getScore(); }
    void setScore(int value) { scoreComponent_->setScore(value); }

    int HitPoints() const { return hitPointsComponent_->CurrentHitPoints; }
    void setHitPoints(int value) { hitPointsComponent_->CurrentHitPoints = value; }

    int GamePhasesPassed() const { return gamePhasesPassed_; }
    System::TimeSpan ElapsedPhaseTime() const { return configurationPhaseTimer_; }

    void LoadContent() override {
        GameScreen::LoadContent();

        viewport_ = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty();

        Vector2 gameOverDimensions = scoreFont_->MeasureString(GameOverText);
        gameOverTextPosition_ =
            Vector2((float)viewport_.getCenterProperty().X - gameOverDimensions.X / 2.0f,
                    (float)viewport_.getCenterProperty().Y - gameOverDimensions.Y / 2.0f);
    }

    // Loads the assets required by the gameplay screen. Called synchronously
    // from LoadingScreen (see missing.md — the original used a background
    // System.Threading.Thread for this).
    void LoadAssets() {
        scoreFont_.emplace(Load<SpriteFont>("Fonts/GameScreenFont28px"));

        LoadTextures();

        animationStore_ = BuildAnimationStore();
        animationStore_.Initialize(GetScreenManager()->getGameProperty().getContentProperty());

        configuration_ = BuildConfiguration();
        gamePhasesPassed_ = -1;
        SwitchConfigurationPhase();

        Game& game = GetScreenManager()->getGameProperty();
        auto& components = game.getComponentsProperty();

        roomComponent_ = std::make_shared<StaticTextureComponent>(game, this, roomTexture_, Vector2::Zero);
        roomComponent_->setDrawOrderProperty(GameConstants::RoomDrawOrder);

        CreateHUDComponents();
        CreateThrowingStars();
        CreateSwordSlashes();

        upperTargetArea_ = BoundingBox(Vector3(GameConstants::UpperTargetAreaTopLeft, 0.0f),
                                       Vector3(GameConstants::UpperTargetAreaBottomRight, 0.0f));
        middleTargetArea_ = BoundingBox(Vector3(GameConstants::MiddleTargetAreaTopLeft, 0.0f),
                                        Vector3(GameConstants::MiddleTargetAreaBottomRight, 0.0f));
        lowerTargetArea_ = BoundingBox(Vector3(GameConstants::LowerTargetAreaTopLeft, 0.0f),
                                       Vector3(GameConstants::LowerTargetAreaBottomRight, 0.0f));

        CreateTargetComponents();
        CreateLaunchedComponents();
        CreateExplosionComponents();
        CreateBambooSliceComponents();

        (void)components;
    }

    // Performs final initialization before the screen is displayed.
    void PreDisplayInitialization() {
        isUpdating_ = true;

        auto& components = GetScreenManager()->getGameProperty().getComponentsProperty();
        components.Add(roomComponent_.get());
        components.Add(hitPointsComponent_.get());
        components.Add(scoreComponent_.get());
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

        if (!isUpdating_)
            return;

        if (moveToHighScore_) {
            isUpdating_ = false;

            for (auto& screen : screensToRemove_)
                screen->ExitScreen();

            AudioManager::PlayMusic("Menu Music");
            return;
        }

        // Make the currently displayed sword slash fade if necessary.
        if (dragPosition_.has_value()) {
            swordSlashCheckTimer_ = swordSlashCheckTimer_ + gameTime.getElapsedGameTimeProperty();

            if (swordSlashCheckTimer_ >= swordSlashCheckInterval_) {
                activeSwordSlash_->Fade(GameConstants::SwordSlashFadeDuration);
                dragPosition_.reset();
            }
        }

        ManageGamePhase(gameTime);
    }

    void HandleInput(InputState& input) override {
        GameScreen::HandleInput(input);

        if (isGameOver_) {
            if (input.IsPauseGame(std::nullopt)) {
                EndGame();
            }

            for (auto& gesture : input.Gestures) {
                if (gesture.getGestureTypeProperty() == GestureType::Tap) {
                    EndGame();
                }
            }
            return;
        }

        if (input.IsPauseGame(std::nullopt)) {
            PauseGame();
        }

        for (auto& gesture : input.Gestures) {
            switch (gesture.getGestureTypeProperty()) {
                case GestureType::Tap:
                    AudioManager::PlaySound("Shuriken");
                    throwingStarComponents_[throwingStarIndex_++]->Throw(gesture.getPositionProperty());
                    if (throwingStarIndex_ >= MaxThrowingStars)
                        throwingStarIndex_ = 0;
                    break;
                case GestureType::FreeDrag:
                    HandleDrag(gesture);
                    break;
                default:
                    break;
            }
        }
    }

    void Draw(const GameTime& gameTime) override {
        GameScreen::Draw(gameTime);

        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();
        spriteBatch.Draw(backgroundTexture_, viewport_, Color::White);
        spriteBatch.End();
    }

    // Pauses the current game.
    void PauseGame() {
        isUpdating_ = false;

        AudioManager::PauseResumeSounds(false);

        auto& components = GetScreenManager()->getGameProperty().getComponentsProperty();
        for (size_t i = 0; i < components.getCountProperty(); i++) {
            if (auto* component = dynamic_cast<RestorableStateComponent*>(components[(int)i])) {
                component->StoreState();
                component->setEnabledProperty(false);
                component->setVisibleProperty(false);
            }
        }

        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("titlescreenBG"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<PauseScreen>(this), std::nullopt);
    }

    // Resumes the game after being paused.
    void ResumeGame() {
        isUpdating_ = true;

        AudioManager::PauseResumeSounds(false);

        auto& components = GetScreenManager()->getGameProperty().getComponentsProperty();
        for (size_t i = 0; i < components.getCountProperty(); i++) {
            if (auto* component = dynamic_cast<RestorableStateComponent*>(components[(int)i])) {
                component->RestoreState();
            }
        }
    }

    void UnloadContent() override {
        auto& components = GetScreenManager()->getGameProperty().getComponentsProperty();

        for (int i = 0; i < (int)components.getCountProperty(); i++) {
            if (dynamic_cast<RestorableStateComponent*>(components[i]) != nullptr) {
                components.RemoveAt(i);
                i--;
            }
        }

        GameScreen::UnloadContent();
    }

    // Switches to the next configuration phase (assumes not already in the final phase).
    void SwitchConfigurationPhase() {
        if ((int)configuration_.Phases.size() > gamePhasesPassed_) {
            gamePhasesPassed_++;
            currentPhase_ = configuration_.Phases[gamePhasesPassed_];

            bambooTimer_ = System::TimeSpan::Zero;
            dynamiteTimer_ = System::TimeSpan::Zero;
            lowerTargetTimer_ = System::TimeSpan::Zero;
            middleTargetTimer_ = System::TimeSpan::Zero;
            upperTargetTimer_ = System::TimeSpan::Zero;
        } else {
            MarkGameOver();
        }
    }

private:
    static constexpr const char* GameOverText = "Game Over";

    static constexpr int MaxThrowingStars = 10;
    static constexpr int MaxSwordSlashes = 3;
    static constexpr int MaxConveyerTargets = 10;
    static constexpr int MaxFallingTargets = 5;
    static constexpr int MaxGoldTargets = 5;
    static constexpr int MaxFallingGoldTargets = 2;
    static constexpr int MaxBamboos = 6;
    static constexpr int MaxDynamites = 3;
    static constexpr int MaxBambooSlices = 5;

    static constexpr float CloseDragDistance = 25.0f;
    static constexpr float AudibleDragDistance = 10.0f;

    // ---- Loading helpers ----

    void LoadTextures() {
        auto& content = GetScreenManager()->getGameProperty().getContentProperty();
        backgroundTexture_ = content.Load<Texture2D>("Textures/Backgrounds/gameplayBG");
        roomTexture_ = content.Load<Texture2D>("Textures/Backgrounds/room");
        bambooTexture_ = content.Load<Texture2D>("Textures/Game Elements/bamboo");
        bambooTopSliceTexture_ = content.Load<Texture2D>("Textures/Game Elements/topSliceHorizontal");
        bambooBottomSliceTexture_ = content.Load<Texture2D>("Textures/Game Elements/bottomSliceHorizontal");
        bambooLeftSliceTexture_ = content.Load<Texture2D>("Textures/Game Elements/leftSliceVertical");
        bambooRightSliceTexture_ = content.Load<Texture2D>("Textures/Game Elements/rightSliceVertical");
        targetTexture_ = content.Load<Texture2D>("Textures/Game Elements/target");
        swordSlashTexture_ = content.Load<Texture2D>("Textures/Game Elements/slice");
        heartTexture_ = content.Load<Texture2D>("Textures/Game Elements/heart");
        emptyHeartTexture_ = content.Load<Texture2D>("Textures/Game Elements/emptyHeart");
    }

    void CreateHUDComponents() {
        Game& game = GetScreenManager()->getGameProperty();

        hitPointsComponent_ = std::make_shared<HitPointsComponent>(game, heartTexture_, emptyHeartTexture_);
        hitPointsComponent_->TotalHitPoints = configuration_.PlayerLives;
        hitPointsComponent_->CurrentHitPoints = configuration_.PlayerLives;
        hitPointsComponent_->setDrawOrderProperty(GameConstants::HUDDrawOrder);

        scoreComponent_ = std::make_shared<ScoreComponent>(game, *scoreFont_);
        scoreComponent_->setScore(0);
        scoreComponent_->setDrawOrderProperty(GameConstants::HUDDrawOrder);
    }

    void CreateThrowingStars() {
        Game& game = GetScreenManager()->getGameProperty();
        auto& components = game.getComponentsProperty();

        throwingStarComponents_.reserve(MaxThrowingStars);
        for (int i = 0; i < MaxThrowingStars; i++) {
            auto star = std::make_shared<ThrowingStar>(game, this, Animation(animationStore_["ThrowingStar"]));
            star->setDrawOrderProperty(GameConstants::ThrowingStarsDrawOrder);
            star->setVisibleProperty(false);
            star->setEnabledProperty(false);
            star->FinishedMoving = [this, raw = star.get()]() { ThrowingStarHit(raw); };

            throwingStarComponents_.push_back(star);
            components.Add(star.get());
        }
    }

    void CreateSwordSlashes() {
        Game& game = GetScreenManager()->getGameProperty();
        auto& components = game.getComponentsProperty();

        swordSlashComponents_.reserve(MaxSwordSlashes);
        for (int i = 0; i < MaxSwordSlashes; i++) {
            auto slash = std::make_shared<SwordSlash>(game, this, swordSlashTexture_);
            slash->setDrawOrderProperty(GameConstants::SwordSlashDrawOrder);
            slash->setVisibleProperty(false);
            slash->setEnabledProperty(false);

            swordSlashComponents_.push_back(slash);
            components.Add(slash.get());
        }
    }

    std::shared_ptr<Target> GetNewConveyerTarget(TargetPosition targetPosition) {
        Game& game = GetScreenManager()->getGameProperty();

        auto target = std::make_shared<Target>(game, this, targetTexture_);
        target->setDrawOrderProperty(GameConstants::TargetDrawOrder);
        target->setVisibleProperty(false);
        target->setEnabledProperty(false);
        target->IsGolden = false;
        target->Designation = targetPosition;
        target->FinishedMoving = [this, raw = target.get()]() { TargetFinishedMoving(raw); };

        return target;
    }

    void CreateTargetComponents() {
        Game& game = GetScreenManager()->getGameProperty();
        auto& components = game.getComponentsProperty();

        for (int i = 0; i < MaxConveyerTargets; i++) {
            auto upper = GetNewConveyerTarget(TargetPosition::Upper);
            auto middle = GetNewConveyerTarget(TargetPosition::Middle);
            auto lower = GetNewConveyerTarget(TargetPosition::Lower);

            upperTargetComponents_.push_back(upper);
            middleTargetComponents_.push_back(middle);
            lowerTargetComponents_.push_back(lower);

            components.Add(upper.get());
            components.Add(middle.get());
            components.Add(lower.get());
        }

        for (int i = 0; i < MaxGoldTargets; i++) {
            auto goldTarget = std::make_shared<Target>(game, this, Animation(animationStore_["GoldTarget"]));
            goldTarget->setDrawOrderProperty(GameConstants::TargetDrawOrder);
            goldTarget->setVisibleProperty(false);
            goldTarget->setEnabledProperty(false);
            goldTarget->IsGolden = true;
            goldTarget->Designation = TargetPosition::Anywhere;
            goldTarget->FinishedMoving = [this, raw = goldTarget.get()]() { TargetFinishedMoving(raw); };

            goldTargetComponents_.push_back(goldTarget);
            components.Add(goldTarget.get());
        }

        for (int i = 0; i < MaxFallingTargets; i++) {
            auto fallingTarget =
                std::make_shared<LaunchedComponent>(game, this, Animation(animationStore_["FallingTarget"]));
            fallingTarget->setDrawOrderProperty(GameConstants::FallingTargetDrawOrder);
            fallingTarget->setVisibleProperty(false);
            fallingTarget->setEnabledProperty(false);
            fallingTarget->NotifyHeight = GameConstants::OffScreenYCoordinate;
            fallingTarget->DroppedPastHeight = [this, raw = fallingTarget.get()]() { FallingTargetDroppedOutOfScreen(raw); };

            fallingTargetComponents_.push_back(fallingTarget);
            components.Add(fallingTarget.get());
        }

        for (int i = 0; i < MaxFallingGoldTargets; i++) {
            auto fallingGoldTarget =
                std::make_shared<LaunchedComponent>(game, this, Animation(animationStore_["FallingGoldTarget"]));
            fallingGoldTarget->setDrawOrderProperty(GameConstants::FallingTargetDrawOrder);
            fallingGoldTarget->setVisibleProperty(false);
            fallingGoldTarget->setEnabledProperty(false);
            fallingGoldTarget->NotifyHeight = GameConstants::OffScreenYCoordinate;
            fallingGoldTarget->DroppedPastHeight = [this, raw = fallingGoldTarget.get()]() { FallingTargetDroppedOutOfScreen(raw); };

            fallingGoldTargetComponents_.push_back(fallingGoldTarget);
            components.Add(fallingGoldTarget.get());
        }
    }

    void CreateLaunchedComponents() {
        Game& game = GetScreenManager()->getGameProperty();
        auto& components = game.getComponentsProperty();

        for (int i = 0; i < MaxBamboos; i++) {
            auto bamboo = std::make_shared<LaunchedComponent>(game, this, bambooTexture_);
            bamboo->setDrawOrderProperty(GameConstants::DefaultDrawOrder);
            bamboo->setVisibleProperty(false);
            bamboo->setEnabledProperty(false);
            bamboo->NotifyHeight = GameConstants::OffScreenYCoordinate;
            bamboo->DroppedPastHeight = [this, raw = bamboo.get()]() { BambooDroppedOutOfScreen(raw); };

            bambooComponents_.push_back(bamboo);
            components.Add(bamboo.get());
        }

        for (int i = 0; i < MaxDynamites; i++) {
            auto dynamite = std::make_shared<LaunchedComponent>(game, this, Animation(animationStore_["Dynamite"]));
            dynamite->setDrawOrderProperty(GameConstants::DefaultDrawOrder);
            dynamite->setVisibleProperty(false);
            dynamite->setEnabledProperty(false);
            dynamite->NotifyHeight = GameConstants::OffScreenYCoordinate;
            dynamite->DroppedPastHeight = [this, raw = dynamite.get()]() { DynamiteDroppedOutOfScreen(raw); };

            dynamiteComponents_.push_back(dynamite);
            components.Add(dynamite.get());
        }
    }

    void CreateExplosionComponents() {
        Game& game = GetScreenManager()->getGameProperty();
        auto& components = game.getComponentsProperty();

        for (int i = 0; i < MaxDynamites; i++) {
            auto explosion =
                std::make_shared<DisappearingAnimationComponent>(game, this, Animation(animationStore_["Explosion"]));
            explosion->setDrawOrderProperty(GameConstants::DefaultDrawOrder);
            explosion->setVisibleProperty(false);
            explosion->setEnabledProperty(false);

            explosionComponents_.push_back(explosion);
            components.Add(explosion.get());
        }
    }

    void CreateBambooSliceComponents() {
        SubCreateBambooSliceComponents(bambooTopSlices_, bambooTopSliceTexture_,
                                       [this](LaunchedComponent* c) { BambooSliceDroppedOutOfScreen(c); });
        SubCreateBambooSliceComponents(bambooBottomSlices_, bambooBottomSliceTexture_,
                                       [this](LaunchedComponent* c) { BambooSliceDroppedOutOfScreen(c); });
        SubCreateBambooSliceComponents(bambooLeftSlices_, bambooLeftSliceTexture_,
                                       [this](LaunchedComponent* c) { BambooSliceDroppedOutOfScreen(c); });
        SubCreateBambooSliceComponents(bambooRightSlices_, bambooRightSliceTexture_,
                                       [this](LaunchedComponent* c) { BambooSliceDroppedOutOfScreen(c); });
    }

    void SubCreateBambooSliceComponents(std::vector<std::shared_ptr<LaunchedComponent>>& componentArray,
                                         Texture2D texture, std::function<void(LaunchedComponent*)> droppedHandler) {
        Game& game = GetScreenManager()->getGameProperty();
        auto& components = game.getComponentsProperty();

        for (int i = 0; i < MaxBambooSlices; i++) {
            auto slice = std::make_shared<LaunchedComponent>(game, this, texture);
            slice->setDrawOrderProperty(GameConstants::DefaultDrawOrder);
            slice->setVisibleProperty(false);
            slice->setEnabledProperty(false);
            slice->NotifyHeight = GameConstants::OffScreenYCoordinate;
            slice->DroppedPastHeight = [droppedHandler, raw = slice.get()]() { droppedHandler(raw); };

            componentArray.push_back(slice);
            components.Add(slice.get());
        }
    }

    // ---- Per-frame game phase management ----

    void ManageGamePhase(GameTime& gameTime) {
        configurationPhaseTimer_ = configurationPhaseTimer_ + gameTime.getElapsedGameTimeProperty();

        if (currentPhase_.Duration >= System::TimeSpan::Zero && configurationPhaseTimer_ >= currentPhase_.Duration) {
            SwitchConfigurationPhase();
        }

        upperTargetTimer_ = upperTargetTimer_ + gameTime.getElapsedGameTimeProperty();
        middleTargetTimer_ = middleTargetTimer_ + gameTime.getElapsedGameTimeProperty();
        lowerTargetTimer_ = lowerTargetTimer_ + gameTime.getElapsedGameTimeProperty();

        upperTargetTimer_ =
            ManagePhaseTargets(gameTime, upperTargetTimer_, currentPhase_.TargetAppearanceIntervals[0],
                               currentPhase_.TargetAppearanceProbabilities[0], upperTargetComponents_,
                               GameConstants::UpperTargetOrigin, GameConstants::UpperTargetDestination);
        middleTargetTimer_ =
            ManagePhaseTargets(gameTime, middleTargetTimer_, currentPhase_.TargetAppearanceIntervals[1],
                               currentPhase_.TargetAppearanceProbabilities[1], middleTargetComponents_,
                               GameConstants::MiddleTargetOrigin, GameConstants::MiddleTargetDestination);
        lowerTargetTimer_ =
            ManagePhaseTargets(gameTime, lowerTargetTimer_, currentPhase_.TargetAppearanceIntervals[2],
                               currentPhase_.TargetAppearanceProbabilities[2], lowerTargetComponents_,
                               GameConstants::LowerTargetOrigin, GameConstants::LowerTargetDestination);

        ManagePhaseBamboos(gameTime);
        ManagePhaseDynamites(gameTime);
    }

    void ManagePhaseDynamites(GameTime& gameTime) {
        dynamiteTimer_ = dynamiteTimer_ + gameTime.getElapsedGameTimeProperty();

        if (dynamiteTimer_ >= currentPhase_.DynamiteAppearanceInterval) {
            dynamiteTimer_ = System::TimeSpan::Zero;

            if (!dynamiteComponents_.empty() && random_.NextDouble() <= currentPhase_.DynamiteAppearanceProbablity) {
                int dynamiteAmount = GetDynamiteAmount();
                dynamiteAmount = std::min(dynamiteAmount, (int)dynamiteComponents_.size());

                AudioManager::PlaySound("Dynamite");

                for (int i = 0; i < dynamiteAmount; i++) {
                    auto launchedDynamite = dynamiteComponents_.back();
                    dynamiteComponents_.pop_back();
                    inAirDynamiteComponents_.push_back(launchedDynamite);

                    Vector2 launchSpeed = GetLaunchSpeed();

                    launchedDynamite->Launch(GetLaunchPosition(), launchSpeed, GameConstants::LaunchAcceleration,
                                             GetLaunchRotation(launchSpeed));
                    launchedDynamite->setEnabledProperty(true);
                    launchedDynamite->setVisibleProperty(true);
                }
            }
        }
    }

    int GetDynamiteAmount() {
        double randomNumber = random_.NextDouble();
        double totalProbability = 0.0;

        for (size_t i = 0; i < currentPhase_.DynamiteAmountProbabilities.size(); i++) {
            totalProbability += currentPhase_.DynamiteAmountProbabilities[i];
            if (randomNumber <= totalProbability)
                return (int)i + 1;
        }

        return (int)currentPhase_.DynamiteAmountProbabilities.size();
    }

    void ManagePhaseBamboos(GameTime& gameTime) {
        bambooTimer_ = bambooTimer_ + gameTime.getElapsedGameTimeProperty();

        if (bambooTimer_ >= currentPhase_.BambooAppearanceInterval) {
            bambooTimer_ = System::TimeSpan::Zero;

            if (!bambooComponents_.empty() && random_.NextDouble() <= currentPhase_.BambooAppearanceProbablity) {
                auto launchedBamboo = bambooComponents_.back();
                bambooComponents_.pop_back();
                inAirBambooComponents_.push_back(launchedBamboo);

                Vector2 launchSpeed = GetLaunchSpeed();

                launchedBamboo->Launch(GetLaunchPosition(), launchSpeed, GameConstants::LaunchAcceleration,
                                       GetLaunchRotation(launchSpeed));
                launchedBamboo->setEnabledProperty(true);
                launchedBamboo->setVisibleProperty(true);
            }
        }
    }

    float GetLaunchRotation(Vector2 launchSpeed) { return 5.0f * launchSpeed.X / 150.0f; }

    Vector2 GetLaunchSpeed() {
        return Vector2(-150.0f + (float)(random_.NextDouble() * 300), -500.0f - (float)(random_.NextDouble() * 150));
    }

    Vector2 GetLaunchPosition() { return Vector2(300.0f + (float)(random_.NextDouble() * 200), 500.0f); }

    // Faithful-to-original quirk (see missing.md): the caller
    // (ManageGamePhase) already added gameTime.ElapsedGameTime to `timer`
    // before calling this, and this method adds it again below -- targets
    // effectively appear on a roughly 2x-faster cadence than
    // Configuration.xml's Interval values specify, in the XNA original too.
    System::TimeSpan ManagePhaseTargets(GameTime& gameTime, System::TimeSpan timer, System::TimeSpan interval,
                                        double probability, std::vector<std::shared_ptr<Target>>& targetStack,
                                        Vector2 origin, Vector2 destination) {
        timer = timer + gameTime.getElapsedGameTimeProperty();

        if (timer >= interval) {
            timer = System::TimeSpan::Zero;

            if (!targetStack.empty() && random_.NextDouble() <= probability) {
                std::shared_ptr<Target> addedTarget;

                if (!goldTargetComponents_.empty() && random_.NextDouble() <= currentPhase_.GoldTargetProbablity) {
                    addedTarget = goldTargetComponents_.back();
                    goldTargetComponents_.pop_back();
                } else {
                    addedTarget = targetStack.back();
                    targetStack.pop_back();
                }

                addedTarget->Move(GameConstants::TargetSpeed, origin, destination);
                addedTarget->setEnabledProperty(true);
                addedTarget->setVisibleProperty(true);

                switch (addedTarget->Designation) {
                    case TargetPosition::Upper:
                        upperTargetsInMotion_.push_back(addedTarget);
                        break;
                    case TargetPosition::Middle:
                        middleTargetsInMotion_.push_back(addedTarget);
                        break;
                    case TargetPosition::Lower:
                        lowerTargetsInMotion_.push_back(addedTarget);
                        break;
                    case TargetPosition::Anywhere:
                        goldTargetsInMotion_.push_back(addedTarget);
                        break;
                }
            }
        }

        return timer;
    }

    std::shared_ptr<SwordSlash> GetSwordSlash() {
        auto result = swordSlashComponents_[swordSlashIndex_++];
        if (swordSlashIndex_ >= MaxSwordSlashes)
            swordSlashIndex_ = 0;
        return result;
    }

    bool SliceComponents(Vector2 origin, Vector2 destination) {
        Line sliceLine(origin, destination);
        bool slicedBamboo = SliceBamboo(sliceLine);
        bool slicedDynamite = SliceDynamite(sliceLine);
        return slicedBamboo || slicedDynamite;
    }

    bool SliceDynamite(const Line& sliceLine) {
        bool result = false;

        for (int dynamiteIndex = 0; dynamiteIndex < (int)inAirDynamiteComponents_.size(); dynamiteIndex++) {
            auto dynamite = inAirDynamiteComponents_[dynamiteIndex];
            auto edges = dynamite->GetEdges();

            for (auto& edge : edges) {
                if (edge.GetIntersection(sliceLine).has_value()) {
                    result = true;

                    AudioManager::PlaySound("Explosion");

                    int lastIndex = (int)inAirDynamiteComponents_.size() - 1;
                    inAirDynamiteComponents_[dynamiteIndex] = inAirDynamiteComponents_[lastIndex];
                    inAirDynamiteComponents_.pop_back();
                    dynamiteIndex--;

                    dynamiteComponents_.push_back(dynamite);
                    dynamite->setEnabledProperty(false);
                    dynamite->setVisibleProperty(false);

                    ShowExplosion(dynamite->Position);

                    setHitPoints(0);
                    MarkGameOver();

                    break;
                }
            }
        }

        return result;
    }

    bool SliceBamboo(const Line& sliceLine) {
        bool result = false;

        for (int bambooIndex = 0; bambooIndex < (int)inAirBambooComponents_.size(); bambooIndex++) {
            auto bamboo = inAirBambooComponents_[bambooIndex];
            auto edges = bamboo->GetEdges();

            bool slicedRight = false;
            bool slicedLeft = false;
            int edgesSliced = 0;

            if (edges[0].GetIntersection(sliceLine).has_value()) edgesSliced++;
            if (edges[1].GetIntersection(sliceLine).has_value()) { slicedRight = true; edgesSliced++; }
            if (edges[2].GetIntersection(sliceLine).has_value()) edgesSliced++;
            if (edges[3].GetIntersection(sliceLine).has_value()) { slicedLeft = true; edgesSliced++; }

            if (edgesSliced >= 2) {
                result = true;

                AudioManager::PlaySound("Bamboo Slice");

                if (slicedLeft && slicedRight)
                    SplitBambooHorizontally(bamboo.get());
                else
                    SplitBambooVertically(bamboo.get());

                int lastIndex = (int)inAirBambooComponents_.size() - 1;
                inAirBambooComponents_[bambooIndex] = inAirBambooComponents_[lastIndex];
                inAirBambooComponents_.pop_back();
                bambooIndex--;

                bambooComponents_.push_back(bamboo);
                bamboo->setEnabledProperty(false);
                bamboo->setVisibleProperty(false);

                scoreComponent_->setScore(scoreComponent_->getScore() + configuration_.PointsPerBamboo);
            }
        }

        return result;
    }

    void SplitBambooVertically(LaunchedComponent* bamboo) {
        Vector2 toLeftHalfCenter(-bamboo->Width() / 4.0f, 0.0f);
        toLeftHalfCenter = Vector2::Transform(toLeftHalfCenter, Matrix::CreateRotationZ(bamboo->Rotation()));
        Vector2 toRightHalfCenter = -toLeftHalfCenter;

        toLeftHalfCenter = toLeftHalfCenter + bamboo->Position;
        toRightHalfCenter = toRightHalfCenter + bamboo->Position;

        auto& leftSlice = bambooLeftSlices_[bambooLeftSliceIndex_++];
        leftSlice->setVisibleProperty(true);
        leftSlice->setEnabledProperty(true);
        leftSlice->Launch(toLeftHalfCenter, bamboo->Velocity() + GetSliceVelocityVariation(), bamboo->Acceleration(),
                          bamboo->Rotation(), bamboo->AngularVelocity() * 0.25f);
        if (bambooLeftSliceIndex_ >= MaxBambooSlices) bambooLeftSliceIndex_ = 0;

        auto& rightSlice = bambooRightSlices_[bambooRightSliceIndex_++];
        rightSlice->setVisibleProperty(true);
        rightSlice->setEnabledProperty(true);
        rightSlice->Launch(toRightHalfCenter, bamboo->Velocity() + GetSliceVelocityVariation(), bamboo->Acceleration(),
                           bamboo->Rotation(), bamboo->AngularVelocity() * 0.25f);
        if (bambooRightSliceIndex_ >= MaxBambooSlices) bambooRightSliceIndex_ = 0;
    }

    void SplitBambooHorizontally(LaunchedComponent* bamboo) {
        Vector2 toTopHalfCenter(0.0f, -bamboo->Height() / 4.0f);
        toTopHalfCenter = Vector2::Transform(toTopHalfCenter, Matrix::CreateRotationZ(bamboo->Rotation()));
        Vector2 toBottomHalfCenter = -toTopHalfCenter;

        toTopHalfCenter = toTopHalfCenter + bamboo->Position;
        toBottomHalfCenter = toBottomHalfCenter + bamboo->Position;

        auto& topSlice = bambooTopSlices_[bambooTopSliceIndex_++];
        topSlice->setVisibleProperty(true);
        topSlice->setEnabledProperty(true);
        topSlice->Launch(toTopHalfCenter, bamboo->Velocity() + GetSliceVelocityVariation(), bamboo->Acceleration(),
                         bamboo->Rotation(), bamboo->AngularVelocity() * 0.25f);
        if (bambooTopSliceIndex_ >= MaxBambooSlices) bambooTopSliceIndex_ = 0;

        auto& bottomSlice = bambooBottomSlices_[bambooBottomSliceIndex_++];
        bottomSlice->setVisibleProperty(true);
        bottomSlice->setEnabledProperty(true);
        bottomSlice->Launch(toBottomHalfCenter, bamboo->Velocity() + GetSliceVelocityVariation(), bamboo->Acceleration(),
                            bamboo->Rotation(), bamboo->AngularVelocity() * 0.25f);
        if (bambooBottomSliceIndex_ >= MaxBambooSlices) bambooBottomSliceIndex_ = 0;
    }

    Vector2 GetSliceVelocityVariation() {
        return Vector2(-30.0f + (float)random_.NextDouble() * 60.0f, -10.0f + (float)random_.NextDouble() * 20.0f);
    }

    void EndGame() {
        if (HighScoreScreen::IsInHighscores(Score())) {
            AudioManager::PlaySound("HighScore");
            GetScreenManager()->AddScreen(std::make_shared<NameEntryScreen>(Score()), std::nullopt);
        } else {
            screensToRemove_ = GetScreenManager()->GetScreens();

            GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("highScoreBG"), std::nullopt);
            GetScreenManager()->AddScreen(std::make_shared<HighScoreScreen>(), std::nullopt);

            moveToHighScore_ = true;
        }
    }

    void HandleDrag(const GestureSample& gesture) {
        float dragDistance = 0.0f;

        if (!dragPosition_.has_value()) {
            swordSlashOrigin_ = gesture.getPositionProperty();
            swordSlashCheckTimer_ = System::TimeSpan::Zero;
            activeSwordSlashHolder_ = GetSwordSlash();
            activeSwordSlash_ = activeSwordSlashHolder_.get();
            activeSwordSlash_->setStretch(0.0f);
            activeSwordSlash_->Reset();
        } else {
            dragDistance = (gesture.getPositionProperty() - dragPosition_.value()).Length();

            if (dragDistance > CloseDragDistance)
                swordSlashCheckTimer_ = System::TimeSpan::Zero;
        }

        dragPosition_ = gesture.getPositionProperty();

        activeSwordSlash_->PositionSlash(swordSlashOrigin_, dragPosition_.value());

        if (!SliceComponents(swordSlashOrigin_, dragPosition_.value()) && dragDistance > AudibleDragDistance) {
            AudioManager::PlaySound("Sword Slash");
        }
    }

    void ShowExplosion(Vector2 position) {
        explosionComponents_[explosionIndex_++]->Show(position);
        if (explosionIndex_ >= MaxDynamites)
            explosionIndex_ = 0;
    }

    void MarkGameOver() {
        isGameOver_ = true;

        AudioManager::PlaySound("Game Over");

        currentPhase_ = GamePhase{};
        currentPhase_.BambooAppearanceProbablity = 0.0;
        currentPhase_.BambooAppearanceInterval = System::TimeSpan::FromSeconds(10);
        currentPhase_.Duration = System::TimeSpan::FromSeconds(-1);
        currentPhase_.DynamiteAppearanceInterval = System::TimeSpan::FromSeconds(10);
        currentPhase_.DynamiteAppearanceProbablity = 0.0;
        currentPhase_.TargetAppearanceIntervals = {System::TimeSpan::FromSeconds(10), System::TimeSpan::FromSeconds(10),
                                                    System::TimeSpan::FromSeconds(10)};
        currentPhase_.TargetAppearanceProbabilities = {0.0, 0.0, 0.0};

        // Kept in a vector (not overwriting a single field) since Game.Components
        // only stores a raw pointer -- XNA's GC keeps the C# original's
        // equivalent object alive via that same reference, but C++ needs an
        // explicit owner for as long as the pointer stays registered.
        auto gameOverText = std::make_shared<TextDisplayComponent>(GetScreenManager()->getGameProperty(), *scoreFont_);
        gameOverText->Position = gameOverTextPosition_;
        gameOverText->Text = GameOverText;
        gameOverText->TextColor = Color::Red;
        gameOverText->setDrawOrderProperty(GameConstants::HUDDrawOrder);

        gameOverTextComponents_.push_back(gameOverText);
        GetScreenManager()->getGameProperty().getComponentsProperty().Add(gameOverText.get());
    }

    // ---- Event handlers ----

    void TargetFinishedMoving(Target* target) {
        target->setEnabledProperty(false);
        target->setVisibleProperty(false);

        RemoveFromMotionListAndReturnToStack(target);
    }

    // Moves target (found by raw pointer) from its "in motion" list back to
    // its available-target stack.
    void RemoveFromMotionListAndReturnToStack(Target* target) {
        auto moveBack = [target](std::vector<std::shared_ptr<Target>>& motionList,
                                  std::vector<std::shared_ptr<Target>>& stack) {
            auto it = std::find_if(motionList.begin(), motionList.end(),
                                   [target](const std::shared_ptr<Target>& t) { return t.get() == target; });
            if (it != motionList.end()) {
                stack.push_back(*it);
                motionList.erase(it);
            }
        };

        switch (target->Designation) {
            case TargetPosition::Upper:
                moveBack(upperTargetsInMotion_, upperTargetComponents_);
                break;
            case TargetPosition::Middle:
                moveBack(middleTargetsInMotion_, middleTargetComponents_);
                break;
            case TargetPosition::Lower:
                moveBack(lowerTargetsInMotion_, lowerTargetComponents_);
                break;
            case TargetPosition::Anywhere:
                moveBack(goldTargetsInMotion_, goldTargetComponents_);
                break;
        }
    }

    void BambooDroppedOutOfScreen(LaunchedComponent* bamboo) {
        bamboo->setEnabledProperty(false);
        bamboo->setVisibleProperty(false);

        for (size_t i = 0; i < inAirBambooComponents_.size(); i++) {
            if (inAirBambooComponents_[i].get() == bamboo) {
                bambooComponents_.push_back(inAirBambooComponents_[i]);
                inAirBambooComponents_.erase(inAirBambooComponents_.begin() + i);
                break;
            }
        }

        setHitPoints(std::max(HitPoints() - 1, 0));

        if (HitPoints() == 0)
            MarkGameOver();
    }

    void BambooSliceDroppedOutOfScreen(LaunchedComponent* bambooSlice) {
        bambooSlice->setEnabledProperty(false);
        bambooSlice->setVisibleProperty(false);
    }

    void DynamiteDroppedOutOfScreen(LaunchedComponent* dynamite) {
        dynamite->setEnabledProperty(false);
        dynamite->setVisibleProperty(false);

        for (size_t i = 0; i < inAirDynamiteComponents_.size(); i++) {
            if (inAirDynamiteComponents_[i].get() == dynamite) {
                dynamiteComponents_.push_back(inAirDynamiteComponents_[i]);
                inAirDynamiteComponents_.erase(inAirDynamiteComponents_.begin() + i);
                break;
            }
        }
    }

    void FallingTargetDroppedOutOfScreen(LaunchedComponent* fallingTarget) {
        fallingTarget->setEnabledProperty(false);
        fallingTarget->setVisibleProperty(false);
    }

    void ThrowingStarHit(ThrowingStar* throwingStar) {
        Vector3 throwingStarPosition3D(throwingStar->Position, 0.0f);

        if (upperTargetArea_.Contains(throwingStarPosition3D) == ContainmentType::Contains) {
            throwingStar->setEnabledProperty(false);
            throwingStar->setVisibleProperty(false);
            CheckForTargetHits(throwingStarPosition3D, upperTargetsInMotion_);
        } else if (middleTargetArea_.Contains(throwingStarPosition3D) == ContainmentType::Contains) {
            throwingStar->setEnabledProperty(false);
            throwingStar->setVisibleProperty(false);
            CheckForTargetHits(throwingStarPosition3D, middleTargetsInMotion_);
        } else if (lowerTargetArea_.Contains(throwingStarPosition3D) == ContainmentType::Contains) {
            throwingStar->setEnabledProperty(false);
            throwingStar->setVisibleProperty(false);
            CheckForTargetHits(throwingStarPosition3D, lowerTargetsInMotion_);
        }

        if (CheckForTargetHits(throwingStarPosition3D, goldTargetsInMotion_)) {
            throwingStar->setVisibleProperty(false);
        }

        // The throwing star remains on screen, "lodged" into something.
        throwingStar->setEnabledProperty(false);
    }

    bool CheckForTargetHits(Vector3 hitPosition, std::vector<std::shared_ptr<Target>>& targets) {
        for (int targetIndex = 0; targetIndex < (int)targets.size(); targetIndex++) {
            Target* target = targets[targetIndex].get();

            if (target->CheckHit(hitPosition)) {
                AudioManager::PlaySound(target->IsGolden ? "Shuriken Hit Gold" : "Shuriken Hit");

                target->setEnabledProperty(false);
                target->setVisibleProperty(false);

                RemoveFromMotionListAndReturnToStack(target);

                DropTargetFromPosition(target->Position, target->IsGolden);

                scoreComponent_->setScore(scoreComponent_->getScore() +
                                          (target->IsGolden ? configuration_.PointsPerGoldTarget
                                                             : configuration_.PointsPerTarget));

                return true;
            }
        }

        return false;
    }

    void DropTargetFromPosition(Vector2 initialPosition, bool isGolden) {
        if (!isGolden) {
            auto& fallingTarget = fallingTargetComponents_[fallingTargetIndex_++];
            fallingTarget->ResetAnimation();
            fallingTarget->Launch(initialPosition, Vector2::Zero, GameConstants::LaunchAcceleration, 0.0f);
            fallingTarget->setEnabledProperty(true);
            fallingTarget->setVisibleProperty(true);

            if (fallingTargetIndex_ >= MaxFallingTargets)
                fallingTargetIndex_ = 0;
        } else {
            auto& fallingGoldTarget = fallingGoldTargetComponents_[fallingGoldTargetIndex_++];
            fallingGoldTarget->ResetAnimation();
            fallingGoldTarget->Launch(initialPosition, Vector2::Zero, GameConstants::LaunchAcceleration, 0.0f);
            fallingGoldTarget->setEnabledProperty(true);
            fallingGoldTarget->setVisibleProperty(true);

            if (fallingGoldTargetIndex_ >= MaxFallingGoldTargets)
                fallingGoldTargetIndex_ = 0;
        }
    }

    // ---- Fields ----

    bool isUpdating_ = false;
    bool isGameOver_ = false;
    bool moveToHighScore_ = false;

    Vector2 gameOverTextPosition_;

    System::Random random_;

    std::optional<SpriteFont> scoreFont_;

    Texture2D backgroundTexture_;
    Texture2D roomTexture_;
    Texture2D bambooTexture_;
    Texture2D bambooTopSliceTexture_;
    Texture2D bambooBottomSliceTexture_;
    Texture2D bambooLeftSliceTexture_;
    Texture2D bambooRightSliceTexture_;
    Texture2D targetTexture_;
    Texture2D swordSlashTexture_;
    Texture2D heartTexture_;
    Texture2D emptyHeartTexture_;
    AnimationStore animationStore_;

    GameConfiguration configuration_;

    Rectangle viewport_;

    std::vector<std::shared_ptr<GameScreen>> screensToRemove_;

    GamePhase currentPhase_;
    System::TimeSpan configurationPhaseTimer_ = System::TimeSpan::Zero;
    int gamePhasesPassed_ = 0;

    std::optional<Vector2> dragPosition_;

    Vector2 swordSlashOrigin_;
    System::TimeSpan swordSlashCheckInterval_ = System::TimeSpan::FromMilliseconds(100);
    System::TimeSpan swordSlashCheckTimer_ = System::TimeSpan::Zero;

    std::shared_ptr<StaticTextureComponent> roomComponent_;
    std::shared_ptr<HitPointsComponent> hitPointsComponent_;
    std::shared_ptr<ScoreComponent> scoreComponent_;
    std::vector<std::shared_ptr<TextDisplayComponent>> gameOverTextComponents_;

    int throwingStarIndex_ = 0;
    std::vector<std::shared_ptr<ThrowingStar>> throwingStarComponents_;

    int swordSlashIndex_ = 0;
    std::vector<std::shared_ptr<SwordSlash>> swordSlashComponents_;
    std::shared_ptr<SwordSlash> activeSwordSlashHolder_;
    SwordSlash* activeSwordSlash_ = nullptr;

    BoundingBox upperTargetArea_;
    BoundingBox middleTargetArea_;
    BoundingBox lowerTargetArea_;

    std::vector<std::shared_ptr<Target>> upperTargetComponents_;
    System::TimeSpan upperTargetTimer_ = System::TimeSpan::Zero;
    std::vector<std::shared_ptr<Target>> upperTargetsInMotion_;

    std::vector<std::shared_ptr<Target>> middleTargetComponents_;
    System::TimeSpan middleTargetTimer_ = System::TimeSpan::Zero;
    std::vector<std::shared_ptr<Target>> middleTargetsInMotion_;

    std::vector<std::shared_ptr<Target>> lowerTargetComponents_;
    System::TimeSpan lowerTargetTimer_ = System::TimeSpan::Zero;
    std::vector<std::shared_ptr<Target>> lowerTargetsInMotion_;

    std::vector<std::shared_ptr<Target>> goldTargetComponents_;
    std::vector<std::shared_ptr<Target>> goldTargetsInMotion_;

    int fallingTargetIndex_ = 0;
    std::vector<std::shared_ptr<LaunchedComponent>> fallingTargetComponents_;

    int fallingGoldTargetIndex_ = 0;
    std::vector<std::shared_ptr<LaunchedComponent>> fallingGoldTargetComponents_;

    std::vector<std::shared_ptr<LaunchedComponent>> bambooComponents_;
    System::TimeSpan bambooTimer_ = System::TimeSpan::Zero;
    std::vector<std::shared_ptr<LaunchedComponent>> inAirBambooComponents_;

    int bambooTopSliceIndex_ = 0;
    std::vector<std::shared_ptr<LaunchedComponent>> bambooTopSlices_;
    int bambooBottomSliceIndex_ = 0;
    std::vector<std::shared_ptr<LaunchedComponent>> bambooBottomSlices_;
    int bambooLeftSliceIndex_ = 0;
    std::vector<std::shared_ptr<LaunchedComponent>> bambooLeftSlices_;
    int bambooRightSliceIndex_ = 0;
    std::vector<std::shared_ptr<LaunchedComponent>> bambooRightSlices_;

    std::vector<std::shared_ptr<LaunchedComponent>> dynamiteComponents_;
    System::TimeSpan dynamiteTimer_ = System::TimeSpan::Zero;
    std::vector<std::shared_ptr<LaunchedComponent>> inAirDynamiteComponents_;

    int explosionIndex_ = 0;
    std::vector<std::shared_ptr<DisappearingAnimationComponent>> explosionComponents_;
};

// ---- cross-referencing method definitions (need GameplayScreen/CountdownScreen complete) ----

inline void MainMenuScreen::LoadContent() {
    instructionsTexture_ = Load<Texture2D>("Textures/Backgrounds/Instructions");
    loadingTexture_ = Load<Texture2D>("Textures/Backgrounds/loading");

    ninjaTexture_ = Load<Texture2D>("Textures/ninja");
    titleTexture_ = Load<Texture2D>("Textures/title");

    if (!HighScoreScreen::HighscoreLoaded()) {
        HighScoreScreen::LoadHighscores();
    }

    AudioManager::LoadSounds();
    AudioManager::LoadMusic();

    for (auto& entry : MenuEntries()) {
        float entryWidth = GetScreenManager()->getFont().MeasureString(entry->Text()).X;
        if (maxEntryWidth_ < entryWidth)
            maxEntryWidth_ = entryWidth;
    }

    MenuScreen::LoadContent();
}

inline void MainMenuScreen::Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) {
    MenuScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

    if (isExiting_) {
        if (!HighScoreScreen::HighscoreSaved()) {
            HighScoreScreen::SaveHighscore();
        } else {
            isExiting_ = false;
            GetScreenManager()->getGameProperty().Exit();
            return;
        }
    }

    if (isMovingToLoading_) {
        for (auto& screen : GetScreenManager()->GetScreens())
            screen->ExitScreen();

        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("Instructions"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<LoadingScreen>(instructionsTexture_, loadingTexture_),
                                     std::nullopt);

        AudioManager::StopMusic();
        return;
    }

    ninjaTimer_ = ninjaTimer_ + gameTime.getElapsedGameTimeProperty();
    titleTimer_ = titleTimer_ + gameTime.getElapsedGameTimeProperty();

    switch (ninjaState_) {
        case ElementState::Invisible:
            if (ninjaTimer_ >= ninjaAppearDelay_) {
                ninjaState_ = ElementState::Appearing;
                ninjaTimer_ = System::TimeSpan::Zero;
            }
            break;
        case ElementState::Appearing:
            ninjaOffset_ = ninjaInitialOffset_ *
                          (float)std::pow(1.0 - ninjaTimer_.getTotalMillisecondsProperty() /
                                                     ninjaAppearDuration_.getTotalMillisecondsProperty(),
                                          2);
            if (ninjaTimer_ > ninjaAppearDuration_)
                ninjaState_ = ElementState::Visible;
            break;
        case ElementState::Visible:
            break;
    }

    switch (titleState_) {
        case ElementState::Invisible:
            if (titleTimer_ >= titleAppearDelay_) {
                titleState_ = ElementState::Appearing;
                titleTimer_ = System::TimeSpan::Zero;
            }
            break;
        case ElementState::Appearing:
            titleOffset_ = titleInitialOffset_ *
                          (float)std::pow(1.0 - titleTimer_.getTotalMillisecondsProperty() /
                                                     titleAppearDuration_.getTotalMillisecondsProperty(),
                                          2);
            if (titleTimer_ > titleAppearDuration_)
                titleState_ = ElementState::Visible;
            break;
        case ElementState::Visible:
            break;
    }
}

inline void MainMenuScreen::StartSelected(PlayerIndex playerIndex) {
    (void)playerIndex;
    AudioManager::PlaySound("Menu Selection");

    // Tombstoning is dropped (see missing.md), so a saved game never exists:
    // always start a fresh game.
    for (auto& screen : GetScreenManager()->GetScreens())
        screen->ExitScreen();

    GetScreenManager()->AddScreen(std::make_shared<LoadingScreen>(instructionsTexture_, loadingTexture_), std::nullopt);

    AudioManager::StopMusic();
}

inline void MainMenuScreen::HighScoreSelected(PlayerIndex playerIndex) {
    (void)playerIndex;
    AudioManager::PlaySound("Menu Selection");

    for (auto& screen : GetScreenManager()->GetScreens())
        screen->ExitScreen();

    GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("highScoreBG"), std::nullopt);
    GetScreenManager()->AddScreen(std::make_shared<HighScoreScreen>(), std::nullopt);
}

inline void LoadingScreen::LoadContent() {
    viewport_ = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty();

    gameplayScreen_ = std::make_shared<GameplayScreen>();
    gameplayScreen_->setScreenManager(GetScreenManager());
}

inline void LoadingScreen::Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) {
    if (loadFinished_ && !IsExiting()) {
        for (auto& screen : GetScreenManager()->GetScreens())
            screen->ExitScreen();

        GetScreenManager()->AddScreen(gameplayScreen_, std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<CountdownScreen>(gameplayScreen_), std::nullopt);
    }

    GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
}

inline void LoadingScreen::LoadResources() {
    isLoading_ = true;
    gameplayScreen_->LoadAssets();
    loadFinished_ = true;
}

inline void PauseScreen::ResumeSelected(PlayerIndex playerIndex) {
    (void)playerIndex;

    if (screenToRestore_ != nullptr) {
        for (auto& screen : GetScreenManager()->GetScreens()) {
            if (dynamic_cast<BackgroundScreen*>(screen.get()) != nullptr) {
                screen->ExitScreen();
            }
        }

        ExitScreen();

        if (auto* gameplay = dynamic_cast<GameplayScreen*>(screenToRestore_)) {
            gameplay->ResumeGame();
        } else if (auto* countdown = dynamic_cast<CountdownScreen*>(screenToRestore_)) {
            countdown->ResumeCountdown();
        }
    }
}

inline void CountdownScreen::Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) {
    GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

    if (!isUpdating_)
        return;

    intervalTimer_ = intervalTimer_ + gameTime.getElapsedGameTimeProperty();

    if (intervalTimer_ >= countdownInterval_) {
        intervalTimer_ = System::TimeSpan::Zero;
        countdownValue_--;
    }

    if (countdownValue_ <= 0) {
        gameplayScreen_->PreDisplayInitialization();
        ExitScreen();
    }
}

} // namespace NinjAcademy
