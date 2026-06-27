#pragma once

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteFont.hpp>
#include <Microsoft/Xna/Framework/Graphics/Model.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>
#include <System/Random.hpp>

#include <optional>
#include <memory>
#include <string>
#include <cmath>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace CameraShake {

// ── Camera ──────────────────────────────────────────────────────────────────

class Camera {
public:
    Vector3 Position = Vector3(0, 0, 0);
    Vector3 Target   = Vector3::Zero;
    Vector3 Up       = Vector3::Up;
    Matrix  Projection;

    Matrix getViewProperty() const {
        Vector3 pos = Position;
        Vector3 tgt = Target;
        if (shaking_) {
            pos = pos + shakeOffset_;
            tgt = tgt + shakeOffset_;
        }
        return Matrix::CreateLookAt(pos, tgt, Up);
    }

    void Shake(float magnitude, float duration) {
        shaking_        = true;
        shakeMagnitude_ = magnitude;
        shakeDuration_  = duration;
        shakeTimer_     = 0.0f;
    }

    void Update(GameTime& gameTime) {
        if (!shaking_) return;

        shakeTimer_ += (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        if (shakeTimer_ >= shakeDuration_) {
            shaking_    = false;
            shakeTimer_ = shakeDuration_;
        }

        float progress  = shakeTimer_ / shakeDuration_;
        float magnitude = shakeMagnitude_ * (1.0f - progress * progress);

        shakeOffset_ = Vector3(NextFloat(), NextFloat(), NextFloat()) * magnitude;
    }

private:
    bool    shaking_        = false;
    float   shakeMagnitude_ = 0.0f;
    float   shakeDuration_  = 0.0f;
    float   shakeTimer_     = 0.0f;
    Vector3 shakeOffset_    = Vector3::Zero;

    System::Random random_;

    float NextFloat() {
        return (float)random_.NextDouble() * 2.0f - 1.0f;
    }
};

// ── CameraShakeGame ──────────────────────────────────────────────────────────

class CameraShakeGame : public Game {
public:
    CameraShakeGame() {
        getContentProperty().setRootDirectoryProperty("Content");
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(1280);
        graphics_->setPreferredBackBufferHeightProperty(720);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "CameraShakeGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        font_        = getContentProperty().Load<SpriteFont>("font");

        ground_ = getContentProperty().Load<Model>("ground");
        tank_   = getContentProperty().Load<Model>("tank");

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        float aspect = (float)vp.getWidthProperty() / (float)vp.getHeightProperty();

        camera_.Position   = Vector3(1000.0f, 1000.0f, 1000.0f);
        camera_.Projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::PiOver4, aspect, 10.0f, 10000.0f);

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        keyboardPrev_ = keyboard_;
        keyboard_     = Keyboard::GetState();

        if (keyboard_.IsKeyDown(Keys::Escape)) { Exit(); return; }

        // A = short shake
        if (keyboard_.IsKeyDown(Keys::A) && keyboardPrev_.IsKeyUp(Keys::A))
            camera_.Shake(25.0f, 0.4f);

        // X = long shake
        if (keyboard_.IsKeyDown(Keys::X) && keyboardPrev_.IsKeyUp(Keys::X))
            camera_.Shake(25.0f, 2.0f);

        camera_.Update(gameTime);
        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255)); // CornflowerBlue

        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);

        Matrix view = camera_.getViewProperty();
        Matrix proj = camera_.Projection;

        ground_->Draw(Matrix::CreateScale(0.1f), view, proj);
        tank_->Draw(Matrix::getIdentityProperty(), view, proj);

        DrawInstructions();

        Game::Draw(gameTime);
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::unique_ptr<SpriteBatch>           spriteBatch_;
    std::optional<SpriteFont>              font_;
    std::optional<Model>                   ground_;
    std::optional<Model>                   tank_;

    Camera camera_;

    KeyboardState keyboard_;
    KeyboardState keyboardPrev_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    void DrawInstructions() {
        if (!font_.has_value()) return;

        static const std::string text = "A - Short shake\nX - Long shake";

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Vector2 pos(20.0f, 20.0f);

        spriteBatch_->Begin();

        // Drop shadow
        spriteBatch_->DrawString(*font_, text, Vector2(pos.X + 1, pos.Y + 1), Color(0, 0, 0, 255));
        spriteBatch_->DrawString(*font_, text, pos, Color(255, 255, 255, 255));

        if (helpTimer_ > 0.0f && helpTexture_.has_value()) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();
    }
};

} // namespace CameraShake
