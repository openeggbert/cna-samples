#pragma once
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include "SpherePrimitive.hpp"
#include "Sphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Devices/Sensors/Accelerometer.hpp"
#include "System/Random.hpp"

namespace Bounce {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using namespace Microsoft::Devices::Sensors;

class BounceGame : public Microsoft::Xna::Framework::Game {
    GraphicsDeviceManager graphics_;

    std::unique_ptr<SpherePrimitive> primitive_;
    std::vector<Sphere> spheres_;

    static constexpr float worldSize        = 3.00f;
    static constexpr float floorPlaneHeight = -1.0f;
    static constexpr int   numSpheres       = 100;
    static constexpr float collisionDamping = 0.75f;

    Color sphereColors_[5] = {
        Color(255, 0,   0,   255),
        Color(0,   128, 0,   255),
        Color(0,   0,   255, 255),
        Color(255, 255, 255, 255),
        Color(0,   0,   0,   255),
    };

    KeyboardState currentKeyboardState_;
    KeyboardState lastKeyboardState_;
    GamePadState  currentGamePadState_;
    GamePadState  lastGamePadState_;

    System::Random random_;
    float accelhistory_[2] = {0.0f, 0.0f};
    Accelerometer accelerometer_;
    bool accelerometerStarted_ = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "BounceGame";
        return name;
    }

    BounceGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void LoadContent() override {
        try {
            accelerometer_.Start();
            accelerometerStarted_ = true;
        } catch (...) {}

        primitive_ = std::make_unique<SpherePrimitive>(getGraphicsDeviceProperty());

        float xpos = -10.0f, zpos = -2.0f, ypos = floorPlaneHeight;
        for (int i = 0; i < numSpheres; i++) {
            Sphere s;
            s.Velocity.X = 0.2f * random_.Next(-10, 10);
            s.Velocity.Z = 0.2f * random_.Next(-10, 10);
            s.Velocity.Y = 0.2f * random_.Next(-3,  3);
            s.SphereColor = sphereColors_[i % 5];
            s.Radius = 0.10f + ((float)random_.Next(100) / 100.0f) * 0.15f;
            s.Position.X = xpos;
            s.Position.Y = ypos + s.Radius * 6.0f;
            s.Position.Z = zpos;
            s.Mass = MathHelper::Pi * (s.Radius * s.Radius * s.Radius);
            spheres_.push_back(s);

            xpos += 1.5f;
            if (xpos > 20.0f) { xpos = -10.0f; zpos -= 1.5f; }
        }
    }

    void Update(GameTime& gameTime) override {
        HandleInput();
        UpdateSpheres(gameTime);
        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255));

        float time   = (float)gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();
        float aspect = getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty();

        Matrix world      = Matrix::CreateFromYawPitchRoll(time * 0.4f, time * 0.7f, time * 1.1f);
        Matrix view       = Matrix::CreateLookAt(Vector3(0.0f, 0.0f, 2.5f), Vector3::Zero, Vector3::Up);
        Matrix projection = Matrix::CreatePerspectiveFieldOfView(1.0f, aspect, 1.0f, 100.0f);
        Matrix worldTranslation = Matrix::CreateTranslation(0.0f, 0.0f, -5.0f);
        world = world * worldTranslation;

        Matrix shadowMatrix = Matrix::getIdentityProperty();
        shadowMatrix.M12 = 0.0f;
        shadowMatrix.M22 = 0.0f;
        shadowMatrix.M23 = 0.0f;

        for (int i = 0; i < numSpheres; i++) {
            Matrix matScale = Matrix::CreateScale(spheres_[i].Radius / 0.5f);
            Matrix worldX   = world * matScale;
            Matrix trans    = Matrix::CreateTranslation(spheres_[i].Position);
            worldX = worldX * trans;
            primitive_->Draw(worldX, view, projection, spheres_[i].SphereColor, false);

            Matrix shadowX = worldX * shadowMatrix;
            shadowX.M42 = -1.0f;
            primitive_->Draw(shadowX, view, projection, Color(0, 0, 0, 255), true);
        }

        Game::Draw(gameTime);
    }

private:
    void SphereCollisionImplicit(Sphere& sphere1, Sphere& sphere2) {
        const float K_ELASTIC = 0.75f;
        Vector3 relativepos = sphere2.Position - sphere1.Position;
        float distance2 = relativepos.LengthSquared();
        float radii = sphere1.Radius + sphere2.Radius;
        if (distance2 >= radii * radii) return;

        float distance = relativepos.Length();
        Vector3 relativeUnit = relativepos * (1.0f / distance);
        Vector3 penetration  = relativeUnit * (radii - distance);

        float mass1 = sphere1.Mass, mass2 = sphere2.Mass;
        float m_inv  = 1.0f / (mass1 + mass2);
        float weight1 = mass1 * m_inv;
        float weight2 = mass2 * m_inv;

        sphere1.Position = sphere1.Position - penetration * weight2;
        sphere2.Position = sphere2.Position + penetration * weight1;

        Vector3 velocity1 = sphere1.Velocity;
        Vector3 velocity2 = sphere2.Velocity;
        Vector3 velocityTotal = velocity1 * weight1 + velocity2 * weight2;
        Vector3 i2 = (velocity2 - velocityTotal) * mass2;
        if (Vector3::Dot(i2, relativeUnit) < 0) {
            Vector3 di = relativeUnit * Vector3::Dot(i2, relativeUnit);
            i2 = i2 - di * (K_ELASTIC + 1.0f);
            sphere1.Velocity = (i2 * -1.0f) / mass1 + velocityTotal;
            sphere2.Velocity = i2 / mass2 + velocityTotal;
        }
    }

    void UpdateSpheres(GameTime& gameTime) {
        Vector3 gravity = Vector3::UnitY * -4.0f;
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        if (accelerometerStarted_) {
            const float limit      = 0.85f;
            const float tiltoffset = 0.76f;
            float shakeForce = 1.0f;

            Vector3 accel = accelerometer_.getCurrentValueProperty().getAccelerationProperty();
            float newMag = accel.Length();

            if (accelhistory_[1] > 1.3f && accelhistory_[0] < accelhistory_[1] && accelhistory_[1] > newMag)
                shakeForce += 10.0f * (accelhistory_[1] - 1.3f) / 3.5f;
            accelhistory_[0] = accelhistory_[1];
            accelhistory_[1] = newMag;

            float rotateX = std::max(std::min(-(accel.Z + tiltoffset), limit), -limit);
            float rotateY = std::max(std::min(accel.Y, limit), -limit);
            gravity = Vector3::Transform(gravity, Matrix::CreateRotationX(rotateX * MathHelper::PiOver2));
            gravity = Vector3::Transform(gravity, Matrix::CreateRotationZ(rotateY * MathHelper::PiOver2));

            if (shakeForce > 1.0f) {
                for (int i = 0; i < numSpheres; i++) {
                    Sphere& s = spheres_[i];
                    if (s.Position.Y <= floorPlaneHeight + s.Radius + 0.01f) {
                        s.Velocity.Y += 0.1f;
                        float speed = s.Velocity.Length();
                        float speedadjust = std::min(std::max(speed, 2.0f), 4.0f);
                        if (speed > 0.0f) s.Velocity = s.Velocity * (speedadjust * shakeForce / speed);
                    }
                }
            }
        }

        for (int i = 0; i < numSpheres; i++) {
            spheres_[i].Position = spheres_[i].Position + spheres_[i].Velocity * elapsed * 0.99f;
            spheres_[i].Velocity = spheres_[i].Velocity + gravity * elapsed;
        }

        for (int i = 0; i < numSpheres; i++)
            for (int j = i + 1; j < numSpheres; j++)
                SphereCollisionImplicit(spheres_[i], spheres_[j]);

        for (int i = 0; i < numSpheres; i++) {
            Sphere& s = spheres_[i];
            if (s.Position.Y < floorPlaneHeight + s.Radius) {
                s.Position.Y = floorPlaneHeight + s.Radius;
                if (s.Velocity.Y < 0) {
                    if (s.Velocity.Y > gravity.Y * elapsed * 2.0f &&
                        s.Velocity.LengthSquared() < 0.5f * 0.5f)
                        s.Velocity.Y = 0.0f;
                    else
                        s.Velocity.Y = -s.Velocity.Y * collisionDamping;
                }
            }
            if (s.Position.X < -worldSize + s.Radius) {
                s.Position.X = -worldSize + s.Radius;
                if (s.Velocity.X < 0) s.Velocity.X = -s.Velocity.X * collisionDamping;
            }
            if (s.Position.X > worldSize - s.Radius) {
                s.Position.X = worldSize - s.Radius;
                if (s.Velocity.X > 0) s.Velocity.X = -s.Velocity.X * collisionDamping;
            }
            if (s.Position.Z < -worldSize + s.Radius) {
                s.Position.Z = -worldSize + s.Radius;
                if (s.Velocity.Z < 0) s.Velocity.Z = -s.Velocity.Z * collisionDamping;
            }
            if (s.Position.Z > worldSize - s.Radius) {
                s.Position.Z = worldSize - s.Radius;
                if (s.Velocity.Z > 0) s.Velocity.Z = -s.Velocity.Z * collisionDamping;
            }
        }
    }

    void HandleInput() {
        lastKeyboardState_  = currentKeyboardState_;
        lastGamePadState_   = currentGamePadState_;
        currentKeyboardState_ = Keyboard::GetState();
        currentGamePadState_  = GamePad::GetState(PlayerIndex::One);

        if (IsPressed(Keys::Escape, Buttons::Back)) Exit();
    }

    bool IsPressed(Keys key, Buttons button) {
        return (currentKeyboardState_.IsKeyDown(key) && lastKeyboardState_.IsKeyUp(key)) ||
               (currentGamePadState_.IsButtonDown(button) && lastGamePadState_.IsButtonUp(button));
    }
};

} // namespace Bounce
