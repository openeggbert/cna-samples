#pragma once

#include <optional>
#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundState.hpp"
#include "Microsoft/Xna/Framework/Media/MediaPlayer.hpp"
#include "Microsoft/Xna/Framework/Media/MediaState.hpp"
#include "Microsoft/Xna/Framework/Media/Song.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Audio;
using namespace Microsoft::Xna::Framework::Input;
using namespace Microsoft::Xna::Framework::Media;

// Simple push button (fires on press, matching XNA TouchDown event behaviour).
struct PushButton {
    Vector2 position;   // top-left draw position
    int width  = 0;
    int height = 0;
    bool isTouched    = false;
    bool wasTriggered = false;

    Rectangle bounds() const {
        return Rectangle((int)position.X, (int)position.Y, width, height);
    }
};

// Horizontal slider handle that can be dragged with the mouse.
struct SliderHandle {
    Vector2 position;   // current center position
    int texW = 0;
    int texH = 0;
    int dragLeft  = 0;  // leftmost allowed center X
    int dragRight = 0;  // rightmost allowed center X
    bool active = false;

    Rectangle bounds() const {
        return Rectangle(
            (int)(position.X - texW / 2),
            (int)(position.Y - texH / 2),
            texW, texH);
    }

    // Returns value in [0, 1] based on position within drag range.
    float scaledValue() const {
        int range = dragRight - dragLeft;
        if (range <= 0) return 0.5f;
        return (position.X - (float)dragLeft) / (float)range;
    }
};

class SoundAndMusicGame : public Microsoft::Xna::Framework::Game {
public:
    SoundAndMusicGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        getContentProperty().setRootDirectoryProperty("Content");
        // Faithful to the XNA original: a portrait 480×800 back buffer.
        graphics_->setPreferredBackBufferWidthProperty(480);
        graphics_->setPreferredBackBufferHeightProperty(800);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "SoundAndMusicGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        auto& content = getContentProperty();
        background_.emplace(content.Load<Texture2D>("Images/bg"));
        sliderStrip_.emplace(content.Load<Texture2D>("Images/sliderStrip"));
        texPlay_.emplace(content.Load<Texture2D>("Images/playButton"));
        texPause_.emplace(content.Load<Texture2D>("Images/pauseButton"));
        texStop_.emplace(content.Load<Texture2D>("Images/stopButton"));
        texHandle_.emplace(content.Load<Texture2D>("Images/sliderHandle"));
        gameFont_.emplace(content.Load<SpriteFont>("Fonts/GameFont"));
        helpTexture_.emplace(content.Load<Texture2D>("help"));

        laser_.emplace(content.Load<SoundEffect>("Sounds/Laser"));
        looped_.emplace(content.Load<SoundEffect>("Sounds/EngineLoop"));
        soundInstance_ = std::make_unique<SoundEffectInstance>(looped_->CreateInstance());
        soundInstance_->setIsLoopedProperty(true);

        song_.emplace(content.Load<Song>("Sounds/Music"));

        initLayout();
    }

    void Update(GameTime& gameTime) override {
        // F1 help overlay
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // Exit
        if (GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back) ||
            Keyboard::GetState().IsKeyDown(Keys::Escape)) {
            Exit();
        }

        auto mouse = Mouse::GetState();
        bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        int mx = mouse.getXProperty();
        int my = mouse.getYProperty();
        Rectangle touchRect(mx - 5, my - 5, 10, 10);

        // Push buttons
        updatePushButton(btnFireForget_, mouseDown, touchRect);
        updatePushButton(btnPlay_, mouseDown, touchRect);
        updatePushButton(btnPause_, mouseDown, touchRect);
        updatePushButton(btnStop_, mouseDown, touchRect);
        updatePushButton(btnSongPlay_, mouseDown, touchRect);
        updatePushButton(btnSongPause_, mouseDown, touchRect);
        updatePushButton(btnSongStop_, mouseDown, touchRect);

        // Slider handles
        if (!mouseDown) {
            activeDrag_ = nullptr;
            handlePan_.active = false;
            handlePitch_.active = false;
            handleVolSound_.active = false;
            handleVolSong_.active = false;
        } else {
            updateSlider(handlePan_, mx, my);
            updateSlider(handlePitch_, mx, my);
            updateSlider(handleVolSound_, mx, my);
            updateSlider(handleVolSong_, mx, my);
        }

        // Apply slider values
        if (soundInstance_) {
            float pan   = (handlePan_.scaledValue() - 0.5f) * 2.0f;
            float pitch = (handlePitch_.scaledValue() - 0.5f) * 2.0f;
            float vol   = handleVolSound_.scaledValue();
            soundInstance_->setPanProperty(pan);
            soundInstance_->setPitchProperty(pitch);
            soundInstance_->setVolumeProperty(vol);
        }
        MediaPlayer::setVolumeProperty(
            MathHelper::Clamp(handleVolSong_.scaledValue(), 0.000001f, 1.0f));

        // React to button presses
        if (btnFireForget_.wasTriggered)
            laser_->Play();

        if (btnPlay_.wasTriggered && soundInstance_) {
            if (soundInstance_->getStateProperty() == SoundState::Paused)
                soundInstance_->Resume();
            else if (soundInstance_->getStateProperty() == SoundState::Stopped)
                soundInstance_->Play();
        }
        if (btnPause_.wasTriggered && soundInstance_) {
            if (soundInstance_->getStateProperty() == SoundState::Playing)
                soundInstance_->Pause();
        }
        if (btnStop_.wasTriggered && soundInstance_) {
            if (soundInstance_->getStateProperty() != SoundState::Stopped)
                soundInstance_->Stop();
        }

        if (btnSongPlay_.wasTriggered && song_) {
            if (MediaPlayer::getStateProperty() == MediaState::Paused)
                MediaPlayer::Resume();
            else if (MediaPlayer::getStateProperty() == MediaState::Stopped) {
                MediaPlayer::setIsRepeatingProperty(true);
                MediaPlayer::Play(&*song_);
            }
        }
        if (btnSongPause_.wasTriggered) {
            if (MediaPlayer::getStateProperty() == MediaState::Playing)
                MediaPlayer::Pause();
        }
        if (btnSongStop_.wasTriggered) {
            if (MediaPlayer::getStateProperty() != MediaState::Stopped)
                MediaPlayer::Stop();
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 255));

        spriteBatch_->Begin();

        // Background fills the full viewport
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Rectangle vpRect(0, 0, vp.getWidthProperty(), vp.getHeightProperty());
        spriteBatch_->Draw(*background_, vpRect, Color(255, 255, 255, 255));

        // Slider strip rails (original XNA 800-px layout coordinates)
        spriteBatch_->Draw(*sliderStrip_, Vector2(96.0f, 364.0f), Color(255, 255, 255, 255));
        spriteBatch_->Draw(*sliderStrip_, Vector2(96.0f, 434.0f), Color(255, 255, 255, 255));
        spriteBatch_->Draw(*sliderStrip_, Vector2(96.0f, 504.0f), Color(255, 255, 255, 255));
        spriteBatch_->Draw(*sliderStrip_, Vector2(96.0f, 742.0f), Color(255, 255, 255, 255));

        // Section labels
        Color blue(0, 168, 255, 255);
        Color white(255, 255, 255, 255);
        spriteBatch_->DrawString(*gameFont_, "Fire and Forget SoundEffect",  Vector2(94.0f,   6.0f),  blue);
        spriteBatch_->DrawString(*gameFont_, "Stored SoundEffectInstance",   Vector2(96.0f, 189.0f),  blue);
        spriteBatch_->DrawString(*gameFont_, "Song",                         Vector2(213.0f, 565.0f), blue);
        spriteBatch_->DrawString(*gameFont_, "Pan",    Vector2(50.0f, 346.0f), white);
        spriteBatch_->DrawString(*gameFont_, "Pitch",  Vector2(37.0f, 416.0f), white);
        spriteBatch_->DrawString(*gameFont_, "Volume", Vector2(8.0f,  486.0f), white);
        spriteBatch_->DrawString(*gameFont_, "Volume", Vector2(8.0f,  725.0f), white);

        // Push buttons
        drawPushButton(btnFireForget_, *texPlay_);
        drawPushButton(btnPlay_,      *texPlay_);
        drawPushButton(btnPause_,     *texPause_);
        drawPushButton(btnStop_,      *texStop_);
        drawPushButton(btnSongPlay_,  *texPlay_);
        drawPushButton(btnSongPause_, *texPause_);
        drawPushButton(btnSongStop_,  *texStop_);

        // Slider handles
        drawSliderHandle(handlePan_);
        drawSliderHandle(handlePitch_);
        drawSliderHandle(handleVolSound_);
        drawSliderHandle(handleVolSong_);

        // F1 help overlay
        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::unique_ptr<SpriteBatch>           spriteBatch_;

    std::optional<Texture2D>   background_;
    std::optional<Texture2D>   sliderStrip_;
    std::optional<Texture2D>   texPlay_;
    std::optional<Texture2D>   texPause_;
    std::optional<Texture2D>   texStop_;
    std::optional<Texture2D>   texHandle_;
    std::optional<SpriteFont>  gameFont_;
    std::optional<Texture2D>   helpTexture_;

    std::optional<SoundEffect>                laser_;
    std::optional<SoundEffect>                looped_;
    std::unique_ptr<SoundEffectInstance>      soundInstance_;
    std::optional<Song>                       song_;

    // Push buttons (fire-and-forget, sound instance controls, song controls)
    PushButton btnFireForget_;
    PushButton btnPlay_, btnPause_, btnStop_;
    PushButton btnSongPlay_, btnSongPause_, btnSongStop_;

    // Slider handles
    SliderHandle handlePan_, handlePitch_, handleVolSound_, handleVolSong_;

    // Currently dragged slider (nullptr if none)
    SliderHandle* activeDrag_ = nullptr;

    // F1 overlay state
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    void initLayout() {
        int pw = texPlay_->getWidthProperty(),   ph = texPlay_->getHeightProperty();
        int hw = texHandle_->getWidthProperty(), hh = texHandle_->getHeightProperty();

        // Push buttons: position is top-left, stored as center in XNA → adjust
        auto setBtn = [&](PushButton& b, float cx, float cy, int w, int h) {
            b.position = Vector2(cx - w / 2.0f, cy - h / 2.0f);
            b.width = w;  b.height = h;
        };

        // Original XNA 480×800 phone layout coordinates.
        setBtn(btnFireForget_,  239.0f, 112.0f, pw, ph);
        setBtn(btnPlay_,        152.0f, 286.0f, pw, ph);
        setBtn(btnPause_,       240.0f, 286.0f,
               texPause_->getWidthProperty(), texPause_->getHeightProperty());
        setBtn(btnStop_,        327.0f, 286.0f,
               texStop_->getWidthProperty(), texStop_->getHeightProperty());
        setBtn(btnSongPlay_,    112.0f, 660.0f, pw, ph);
        setBtn(btnSongPause_,   240.0f, 660.0f,
               texPause_->getWidthProperty(), texPause_->getHeightProperty());
        setBtn(btnSongStop_,    367.0f, 660.0f,
               texStop_->getWidthProperty(), texStop_->getHeightProperty());

        // Slider handles: XNA stores center; drag range is [left, left+width]
        auto setSlider = [&](SliderHandle& s, float cx, float cy, int dragX, int dragW) {
            s.position = Vector2(cx, cy);
            s.texW = hw;  s.texH = hh;
            s.dragLeft  = dragX;
            s.dragRight = dragX + dragW;
        };

        setSlider(handlePan_,      280.0f, 364.0f, 120, 300);
        setSlider(handlePitch_,    280.0f, 434.0f, 120, 300);
        setSlider(handleVolSound_, 360.0f, 505.0f, 120, 300);
        setSlider(handleVolSong_,  300.0f, 743.0f, 120, 300);
    }

    void updatePushButton(PushButton& btn, bool mouseDown, const Rectangle& touchRect) {
        btn.wasTriggered = false;
        if (!btn.isTouched && mouseDown && btn.bounds().Intersects(touchRect)) {
            btn.isTouched    = true;
            btn.wasTriggered = true;
        } else if (btn.isTouched && !mouseDown) {
            btn.isTouched = false;
        }
    }

    void updateSlider(SliderHandle& handle, int mx, int my) {
        if (activeDrag_ == &handle) {
            float newX = MathHelper::Clamp(
                (float)mx, (float)handle.dragLeft, (float)handle.dragRight);
            handle.position = Vector2(newX, handle.position.Y);
            return;
        }
        if (activeDrag_ != nullptr) return;
        Rectangle touchRect(mx - 5, my - 5, 10, 10);
        if (!handle.active && handle.bounds().Intersects(touchRect)) {
            handle.active = true;
            activeDrag_   = &handle;
        }
    }

    void drawPushButton(const PushButton& btn, const Texture2D& tex) {
        Color tint = btn.isTouched
            ? Color(169, 169, 169, 255)
            : Color(255, 255, 255, 255);
        spriteBatch_->Draw(tex, btn.position, tint);
    }

    void drawSliderHandle(const SliderHandle& handle) {
        spriteBatch_->Draw(
            *texHandle_,
            Vector2(handle.position.X - handle.texW / 2.0f,
                    handle.position.Y - handle.texH / 2.0f),
            Color(255, 255, 255, 255));
    }
};
