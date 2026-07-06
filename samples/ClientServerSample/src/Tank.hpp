#pragma once

#include <Microsoft/Xna/Framework/Content/ContentManager.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>

#include <cmath>
#include <algorithm>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Content;

namespace ClientServer {

// Each player controls a tank, which they can drive around the screen. This class
// implements the logic for moving and drawing the tank, and responds to input that is
// passed in from outside. The Tank class does not implement any networking
// functionality, however: that is all handled by the main game class.
class Tank {
public:
    // The current position and rotation of the tank.
    Vector2 Position;
    Vector2 Velocity;
    float TankRotation = -MathHelper::PiOver2;
    float TurretRotation = -MathHelper::PiOver2;

    // Input controls can be read from keyboard, gamepad, or the network.
    Vector2 TankInput;
    Vector2 TurretInput;

    Tank(int gamerIndex, ContentManager& content, int screenWidth, int screenHeight) {
        // Use the gamer index to compute a starting position, so each player starts in
        // a different place as opposed to all on top of each other.
        Position.X = (float)(screenWidth / 4 + (gamerIndex % 5) * screenWidth / 8);
        Position.Y = (float)(screenHeight / 4 + (gamerIndex / 5) * screenHeight / 5);

        tankTexture_ = content.Load<Texture2D>("Tank");
        turretTexture_ = content.Load<Texture2D>("Turret");

        screenSize_ = Vector2((float)screenWidth, (float)screenHeight);
    }

    // Moves the tank in response to the current input settings.
    void Update() {
        // Gradually turn the tank and turret to face the requested direction.
        TankRotation = TurnToFace(TankRotation, TankInput, kTankTurnRate);
        TurretRotation = TurnToFace(TurretRotation, TurretInput, kTurretTurnRate);

        // How close the desired direction is the tank facing?
        Vector2 tankForward((float)std::cos(TankRotation), (float)std::sin(TankRotation));
        Vector2 targetForward(TankInput.X, -TankInput.Y);

        float facingForward = Vector2::Dot(tankForward, targetForward);

        // If we have finished turning, also start moving forward.
        if (facingForward > 0)
            Velocity = Velocity + tankForward * facingForward * facingForward * kTankSpeed;

        // Update the position and velocity.
        Position = Position + Velocity;
        Velocity = Velocity * kTankFriction;

        // Clamp so the tank cannot drive off the edge of the screen.
        Position = Vector2::Clamp(Position, Vector2::Zero, screenSize_);
    }

    // Draws the tank and turret.
    void Draw(SpriteBatch& spriteBatch) {
        Vector2 origin((float)(tankTexture_.getWidthProperty() / 2),
                        (float)(tankTexture_.getHeightProperty() / 2));

        spriteBatch.Draw(tankTexture_, Position, std::nullopt, Color(255, 255, 255, 255),
                          TankRotation, origin, 1.0f, SpriteEffects::None, 0.0f);

        spriteBatch.Draw(turretTexture_, Position, std::nullopt, Color(255, 255, 255, 255),
                          TurretRotation, origin, 1.0f, SpriteEffects::None, 0.0f);
    }

private:
    // Constants control how fast the tank moves and turns.
    static constexpr float kTankTurnRate = 0.01f;
    static constexpr float kTurretTurnRate = 0.03f;
    static constexpr float kTankSpeed = 0.3f;
    static constexpr float kTankFriction = 0.9f;

    Texture2D tankTexture_;
    Texture2D turretTexture_;
    Vector2 screenSize_;

    // Gradually rotates the tank to face the specified direction.
    static float TurnToFace(float rotation, Vector2 target, float turnRate) {
        if (target == Vector2::Zero)
            return rotation;

        float angle = (float)std::atan2(-target.Y, target.X);
        float difference = rotation - angle;

        while (difference > MathHelper::Pi)
            difference -= MathHelper::TwoPi;
        while (difference < -MathHelper::Pi)
            difference += MathHelper::TwoPi;

        turnRate *= std::abs(difference);

        if (difference < 0)
            return rotation + std::min(turnRate, -difference);
        else
            return rotation - std::min(turnRate, difference);
    }
};

} // namespace ClientServer
