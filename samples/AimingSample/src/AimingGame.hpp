#pragma once
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"

namespace Aiming {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class AimingGame : public Microsoft::Xna::Framework::Game {
    static constexpr float CatSpeed            = 10.0f;
    static constexpr float SpotlightTurnSpeed  = 0.025f;

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;

    std::optional<Texture2D> spotlightTexture_;
    Vector2 spotlightPosition_;
    Vector2 spotlightOrigin_;
    float   spotlightAngle_ = 0.0f;

    std::optional<Texture2D> catTexture_;
    Vector2 catPosition_;
    Vector2 catOrigin_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "AimingGame";
        return name;
    }

    AimingGame() : graphics_(this) {
        graphics_.setPreferredBackBufferWidthProperty(853);
        graphics_.setPreferredBackBufferHeightProperty(480);
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void Initialize() override {
        Game::Initialize();
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        spotlightPosition_ = Vector2((float)(vp.getWidthProperty()  / 2),
                                    (float)(vp.getHeightProperty() / 2));
        catPosition_       = Vector2((float)(vp.getWidthProperty()  / 4),
                                    (float)(vp.getHeightProperty() / 2));
    }

    void LoadContent() override {
        spriteBatch_       = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        spotlightTexture_.emplace(getContentProperty().Load<Texture2D>("spotlight"));
        catTexture_.emplace(getContentProperty().Load<Texture2D>("cat"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        spotlightOrigin_ = Vector2(0.0f, (float)(spotlightTexture_->getHeightProperty() / 2));
        catOrigin_       = Vector2((float)(catTexture_->getWidthProperty()  / 2),
                                   (float)(catTexture_->getHeightProperty() / 2));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        catPosition_.X = MathHelper::Clamp(catPosition_.X, 0.0f, (float)vp.getWidthProperty());
        catPosition_.Y = MathHelper::Clamp(catPosition_.Y, 0.0f, (float)vp.getHeightProperty());

        spotlightAngle_ = TurnToFace(spotlightPosition_, catPosition_,
                                     spotlightAngle_, SpotlightTurnSpeed);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 255));

        spriteBatch_->Begin();
        spriteBatch_->Draw(*catTexture_, catPosition_, std::nullopt,
                           Color(255, 255, 255, 255), 0.0f, catOrigin_,
                           1.0f, SpriteEffects::None, 0.0f);
        spriteBatch_->Draw(*spotlightTexture_, spotlightPosition_, std::nullopt,
                           Color(255, 255, 255, 255), spotlightAngle_, spotlightOrigin_,
                           1.0f, SpriteEffects::None, 0.0f);
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
        KeyboardState kb  = Keyboard::GetState();
        GamePadState  pad = GamePad::GetState(PlayerIndex::One);
        MouseState    ms  = Mouse::GetState();

        if (kb.IsKeyDown(Keys::Escape) || pad.IsButtonDown(Buttons::Back))
            Exit();

        Vector2 catMovement(
            pad.getThumbSticksProperty().getLeftProperty().X,
           -pad.getThumbSticksProperty().getLeftProperty().Y);

        if (kb.IsKeyDown(Keys::Left)  || pad.IsButtonDown(Buttons::DPadLeft))  catMovement.X -= 1.0f;
        if (kb.IsKeyDown(Keys::Right) || pad.IsButtonDown(Buttons::DPadRight)) catMovement.X += 1.0f;
        if (kb.IsKeyDown(Keys::Up)    || pad.IsButtonDown(Buttons::DPadUp))    catMovement.Y -= 1.0f;
        if (kb.IsKeyDown(Keys::Down)  || pad.IsButtonDown(Buttons::DPadDown))  catMovement.Y += 1.0f;

        float smoothStop = 1.0f;
        Vector2 mousePos((float)ms.getXProperty(), (float)ms.getYProperty());
        if (ms.getLeftButtonProperty() == ButtonState::Pressed
            && Vector2::Distance(mousePos, catPosition_) > 0.5f)
        {
            catMovement = mousePos - catPosition_;
            float delta = CatSpeed - MathHelper::Clamp(catMovement.Length(), 0.0f, CatSpeed);
            smoothStop  = 1.0f - delta / CatSpeed;
        }

        if (catMovement.LengthSquared() > 0.0f)
            catMovement.Normalize();

        catPosition_ = catPosition_ + catMovement * CatSpeed * smoothStop;
    }

    static float TurnToFace(Vector2 position, Vector2 faceThis,
                             float currentAngle, float turnSpeed) {
        float x = faceThis.X - position.X;
        float y = faceThis.Y - position.Y;
        float desiredAngle = std::atan2(y, x);
        float difference   = WrapAngle(desiredAngle - currentAngle);
        difference = MathHelper::Clamp(difference, -turnSpeed, turnSpeed);
        return WrapAngle(currentAngle + difference);
    }

    static float WrapAngle(float r) {
        while (r < -MathHelper::Pi)  r += MathHelper::TwoPi;
        while (r >  MathHelper::Pi)  r -= MathHelper::TwoPi;
        return r;
    }
};

} // namespace Aiming
