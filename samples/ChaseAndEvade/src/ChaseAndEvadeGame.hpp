#pragma once
#include <algorithm>
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
#include "System/Random.hpp"

namespace ChaseAndEvade {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

enum class TankAiState { Chasing, Caught, Wander };
enum class MouseAiState { Evading, Wander };

class ChaseAndEvadeGame : public Microsoft::Xna::Framework::Game {
    static constexpr float MaxCatSpeed         = 7.5f;
    static constexpr float MaxTankSpeed        = 5.0f;
    static constexpr float TankTurnSpeed       = 0.10f;
    static constexpr float TankChaseDistance   = 250.0f;
    static constexpr float TankCaughtDistance  = 60.0f;
    static constexpr float TankHysteresis      = 15.0f;
    static constexpr float MaxMouseSpeed       = 8.5f;
    static constexpr float MouseTurnSpeed      = 0.20f;
    static constexpr float MouseEvadeDistance  = 200.0f;
    static constexpr float MouseHysteresis     = 60.0f;

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;

    std::optional<Texture2D> tankTexture_;
    Vector2 tankTextureCenter_;
    Vector2 tankPosition_;
    TankAiState tankState_ = TankAiState::Wander;
    float tankOrientation_ = 0.0f;
    Vector2 tankWanderDirection_;

    std::optional<Texture2D> catTexture_;
    Vector2 catTextureCenter_;
    Vector2 catPosition_;

    std::optional<Texture2D> mouseTexture_;
    Vector2 mouseTextureCenter_;
    Vector2 mousePosition_;
    MouseAiState mouseState_ = MouseAiState::Wander;
    float mouseOrientation_ = 0.0f;
    Vector2 mouseWanderDirection_;

    System::Random random_;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "ChaseAndEvadeGame";
        return name;
    }

    ChaseAndEvadeGame() : graphics_(this) {
        graphics_.setPreferredBackBufferWidthProperty(853);
        graphics_.setPreferredBackBufferHeightProperty(480);
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void Initialize() override {
        Game::Initialize();
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int w = vp.getWidthProperty(), h = vp.getHeightProperty();
        tankPosition_  = Vector2((float)(w / 4),     (float)(h / 2));
        catPosition_   = Vector2((float)(w / 2),     (float)(h / 2));
        mousePosition_ = Vector2((float)(3 * w / 4), (float)(h / 2));
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        tankTexture_.emplace(getContentProperty().Load<Texture2D>("Tank"));
        catTexture_.emplace(getContentProperty().Load<Texture2D>("Cat"));
        mouseTexture_.emplace(getContentProperty().Load<Texture2D>("Mouse"));
        tankTextureCenter_  = Vector2((float)(tankTexture_->getWidthProperty()  / 2), (float)(tankTexture_->getHeightProperty()  / 2));
        catTextureCenter_   = Vector2((float)(catTexture_->getWidthProperty()   / 2), (float)(catTexture_->getHeightProperty()   / 2));
        mouseTextureCenter_ = Vector2((float)(mouseTexture_->getWidthProperty() / 2), (float)(mouseTexture_->getHeightProperty() / 2));
    }

    void Update(GameTime& gameTime) override {
        HandleInput();
        UpdateTank();
        UpdateMouse();
        tankPosition_  = ClampToViewport(tankPosition_);
        catPosition_   = ClampToViewport(catPosition_);
        mousePosition_ = ClampToViewport(mousePosition_);
        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255));
        spriteBatch_->Begin();
        spriteBatch_->Draw(*tankTexture_,  tankPosition_,  std::nullopt, Color(255,255,255,255), tankOrientation_,  tankTextureCenter_,  1.0f, SpriteEffects::None, 0.0f);
        spriteBatch_->Draw(*catTexture_,   catPosition_,   std::nullopt, Color(255,255,255,255), 0.0f,              catTextureCenter_,   1.0f, SpriteEffects::None, 0.0f);
        spriteBatch_->Draw(*mouseTexture_, mousePosition_, std::nullopt, Color(255,255,255,255), mouseOrientation_, mouseTextureCenter_, 1.0f, SpriteEffects::None, 0.0f);
        spriteBatch_->End();
        Game::Draw(gameTime);
    }

private:
    Vector2 ClampToViewport(Vector2 v) {
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        v.X = MathHelper::Clamp(v.X, 0.0f, (float)vp.getWidthProperty());
        v.Y = MathHelper::Clamp(v.Y, 0.0f, (float)vp.getHeightProperty());
        return v;
    }

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
        Vector2 clickPos((float)ms.getXProperty(), (float)ms.getYProperty());
        if (ms.getLeftButtonProperty() == ButtonState::Pressed
            && Vector2::Distance(clickPos, catPosition_) > 0.5f)
        {
            catMovement = clickPos - catPosition_;
            float delta = MaxCatSpeed - MathHelper::Clamp(catMovement.Length(), 0.0f, MaxCatSpeed);
            smoothStop  = 1.0f - delta / MaxCatSpeed;
        }

        if (catMovement.LengthSquared() > 0.0f)
            catMovement.Normalize();

        catPosition_ = catPosition_ + catMovement * MaxCatSpeed * smoothStop;
    }

    void UpdateMouse() {
        float dist = Vector2::Distance(mousePosition_, catPosition_);
        if      (dist > MouseEvadeDistance + MouseHysteresis) mouseState_ = MouseAiState::Wander;
        else if (dist < MouseEvadeDistance - MouseHysteresis) mouseState_ = MouseAiState::Evading;

        float speed;
        if (mouseState_ == MouseAiState::Evading) {
            Vector2 seekPos = mousePosition_ + mousePosition_ - catPosition_;
            mouseOrientation_ = TurnToFace(mousePosition_, seekPos, mouseOrientation_, MouseTurnSpeed);
            speed = MaxMouseSpeed;
        } else {
            Wander(mousePosition_, mouseWanderDirection_, mouseOrientation_, MouseTurnSpeed);
            speed = 0.25f * MaxMouseSpeed;
        }
        Vector2 heading(std::cos(mouseOrientation_), std::sin(mouseOrientation_));
        mousePosition_ = mousePosition_ + heading * speed;
    }

    void UpdateTank() {
        float chaseThreshold  = TankChaseDistance;
        float caughtThreshold = TankCaughtDistance;
        if      (tankState_ == TankAiState::Wander)   { chaseThreshold  -= TankHysteresis / 2.0f; }
        else if (tankState_ == TankAiState::Chasing)  { chaseThreshold  += TankHysteresis / 2.0f; caughtThreshold -= TankHysteresis / 2.0f; }
        else if (tankState_ == TankAiState::Caught)   { caughtThreshold += TankHysteresis / 2.0f; }

        float dist = Vector2::Distance(tankPosition_, catPosition_);
        if      (dist > chaseThreshold)  tankState_ = TankAiState::Wander;
        else if (dist > caughtThreshold) tankState_ = TankAiState::Chasing;
        else                             tankState_ = TankAiState::Caught;

        float speed;
        if (tankState_ == TankAiState::Chasing) {
            tankOrientation_ = TurnToFace(tankPosition_, catPosition_, tankOrientation_, TankTurnSpeed);
            speed = MaxTankSpeed;
        } else if (tankState_ == TankAiState::Wander) {
            Wander(tankPosition_, tankWanderDirection_, tankOrientation_, TankTurnSpeed);
            speed = 0.25f * MaxTankSpeed;
        } else {
            speed = 0.0f;
        }
        Vector2 heading(std::cos(tankOrientation_), std::sin(tankOrientation_));
        tankPosition_ = tankPosition_ + heading * speed;
    }

    void Wander(const Vector2& position, Vector2& wanderDirection,
                float& orientation, float turnSpeed) {
        wanderDirection.X = wanderDirection.X + MathHelper::Lerp(-0.25f, 0.25f, (float)random_.NextDouble());
        wanderDirection.Y = wanderDirection.Y + MathHelper::Lerp(-0.25f, 0.25f, (float)random_.NextDouble());
        if (wanderDirection.LengthSquared() > 0.0f)
            wanderDirection.Normalize();
        orientation = TurnToFace(position, position + wanderDirection, orientation, 0.15f * turnSpeed);

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Vector2 screenCenter((float)(vp.getWidthProperty() / 2), (float)(vp.getHeightProperty() / 2));
        float distFromCenter = Vector2::Distance(screenCenter, position);
        float maxDist = std::min((float)vp.getHeightProperty(), (float)vp.getWidthProperty()) / 2.0f;
        float norm = distFromCenter / maxDist;
        float turnToCenter = 0.3f * norm * norm * turnSpeed;
        orientation = TurnToFace(position, screenCenter, orientation, turnToCenter);
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
        while (r < -MathHelper::Pi) r += MathHelper::TwoPi;
        while (r >  MathHelper::Pi) r -= MathHelper::TwoPi;
        return r;
    }
};

} // namespace ChaseAndEvade
