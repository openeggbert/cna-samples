#pragma once
#include <memory>
#include <optional>
#include <string>
#include "Level.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/TitleContainer.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Media/MediaPlayer.hpp"
#include "Microsoft/Xna/Framework/Media/Song.hpp"
#include "System/TimeSpan.hpp"

namespace Platformer {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using namespace Microsoft::Xna::Framework::Media;

class PlatformerGame : public Microsoft::Xna::Framework::Game {
    GraphicsDeviceManager graphics_;
    SpriteBatch* spriteBatch_ = nullptr;

    std::optional<SpriteFont>  hudFont_;
    std::optional<Texture2D>   winOverlay_;
    std::optional<Texture2D>   loseOverlay_;
    std::optional<Texture2D>   diedOverlay_;

    int levelIndex_ = -1;
    std::unique_ptr<Level> level_;
    bool wasContinuePressed_ = false;

    inline static const System::TimeSpan WarningTime_ = System::TimeSpan::FromSeconds(30.0);

    GamePadState  gamePadState_;
    KeyboardState keyboardState_;

    static constexpr int numberOfLevels = 3;

    std::optional<Song> music_;

    // --- F1 help overlay ---
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "PlatformerGame";
        return name;
    }

    PlatformerGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void LoadContent() override {
        spriteBatch_ = new SpriteBatch(getGraphicsDeviceProperty());

        hudFont_.emplace(getContentProperty().Load<SpriteFont>("Fonts/Hud"));

        winOverlay_.emplace( getContentProperty().Load<Texture2D>("Overlays/you_win"));
        loseOverlay_.emplace(getContentProperty().Load<Texture2D>("Overlays/you_lose"));
        diedOverlay_.emplace(getContentProperty().Load<Texture2D>("Overlays/you_died"));

        try {
            music_.emplace(getContentProperty().Load<Song>("Sounds/Music"));
            MediaPlayer::setIsRepeatingProperty(true);
            MediaPlayer::Play(&*music_);
        } catch (...) {}

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
        LoadNextLevel();
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();
        level_->Update(gameTime, keyboardState_, gamePadState_);
        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255));

        spriteBatch_->Begin();
        // Draw needs non-const GameTime for AnimationPlayer — cast is safe here
        GameTime& gt = const_cast<GameTime&>(gameTime);
        level_->Draw(gt, *spriteBatch_);
        DrawHud();
        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }
        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    void HandleInput() {
        keyboardState_ = Keyboard::GetState();
        gamePadState_  = GamePad::GetState(PlayerIndex::One);

        if (gamePadState_.IsButtonDown(Buttons::Back))
            Exit();

        bool continuePressed =
            keyboardState_.IsKeyDown(Keys::Space) ||
            gamePadState_.IsButtonDown(Buttons::A);

        if (!wasContinuePressed_ && continuePressed) {
            Player* p = level_->getPlayerProperty();
            if (!p->getIsAliveProperty()) {
                level_->StartNewLife();
            } else if (level_->getTimeRemainingProperty() == System::TimeSpan::Zero) {
                if (level_->getReachedExitProperty())
                    LoadNextLevel();
                else
                    ReloadCurrentLevel();
            }
        }

        wasContinuePressed_ = continuePressed;
    }

    void LoadNextLevel() {
        levelIndex_ = (levelIndex_ + 1) % numberOfLevels;
        level_.reset();
        std::string levelPath = std::string("Content/Levels/") + std::to_string(levelIndex_) + ".txt";
        auto stream = TitleContainer::OpenStream(levelPath);
        level_ = std::make_unique<Level>(&getServicesProperty(), *stream, levelIndex_,
                                         getGraphicsDeviceProperty());
    }

    void ReloadCurrentLevel() {
        --levelIndex_;
        LoadNextLevel();
    }

    void DrawHud() {
        Rectangle titleSafeArea = getGraphicsDeviceProperty().getViewportProperty().getTitleSafeAreaProperty();
        Vector2 hudLocation((float)titleSafeArea.X, (float)titleSafeArea.Y);
        Vector2 center(
            titleSafeArea.X + titleSafeArea.Width  / 2.0f,
            titleSafeArea.Y + titleSafeArea.Height / 2.0f
        );

        System::TimeSpan tr = level_->getTimeRemainingProperty();
        auto pad2 = [](int v) { return (v < 10 ? std::string("0") : std::string("")) + std::to_string(v); };
        std::string timeString = std::string("TIME: ") + pad2(tr.getMinutesProperty()) + ":" + pad2(tr.getSecondsProperty());

        Color timeColor = (tr > WarningTime_ || level_->getReachedExitProperty() ||
                           (int)tr.getTotalSecondsProperty() % 2 == 0)
            ? Color(255, 255, 0, 255) : Color(255, 0, 0, 255);
        DrawShadowedString(*hudFont_, timeString, hudLocation, timeColor);

        float timeHeight = hudFont_->MeasureString(timeString).Y;
        DrawShadowedString(*hudFont_,
                           std::string("SCORE: ") + std::to_string(level_->getScoreProperty()),
                           hudLocation + Vector2(0.0f, timeHeight * 1.2f),
                           Color(255, 255, 0, 255));

        Texture2D* status = nullptr;
        if (tr == System::TimeSpan::Zero) {
            status = level_->getReachedExitProperty() ? &*winOverlay_ : &*loseOverlay_;
        } else if (!level_->getPlayerProperty()->getIsAliveProperty()) {
            status = &*diedOverlay_;
        }

        if (status) {
            Vector2 statusSize((float)status->getWidthProperty(), (float)status->getHeightProperty());
            spriteBatch_->Draw(*status, center - statusSize / 2.0f, Color(255, 255, 255, 255));
        }
    }

    void DrawShadowedString(const SpriteFont& font, const std::string& value,
                             Vector2 position, Color color) {
        spriteBatch_->DrawString(font, value, position + Vector2(1.0f, 1.0f), Color(0, 0, 0, 255));
        spriteBatch_->DrawString(font, value, position, color);
    }
};

} // namespace Platformer
