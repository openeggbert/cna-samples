#pragma once
#include <cmath>
#include <memory>
#include <optional>
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
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
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

    // 2D overlay
    std::unique_ptr<SpriteBatch>  spriteBatch_;
    std::optional<Texture2D>      pixel_;

    static constexpr float worldSize        = 3.00f;
    static constexpr float floorPlaneHeight = -1.0f;
    static constexpr int   numSpheres       = 100;
    static constexpr float collisionDamping = 0.75f;
    static constexpr float tiltLimit        = 0.85f;
    static constexpr float tiltOffset       = 0.76f;

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

    // Desktop-only simulated accelerometer: accelY=left/right, accelZ=fwd/back
    // Default accelZ=-tiltOffset so that effective rotateX=0 → gravity straight down
    Vector3 simAccel_{0.0f, 0.0f, -tiltOffset};

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

        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        pixel_.emplace(getGraphicsDeviceProperty(), 1, 1);
        Color white(255, 255, 255, 255);
        pixel_->SetData(&white, 1);

        primitive_ = std::make_unique<SpherePrimitive>(getGraphicsDeviceProperty());

        float xpos = -10.0f, zpos = -2.0f, ypos = floorPlaneHeight;
        for (int i = 0; i < numSpheres; i++) {
            Sphere s;
            s.Velocity.X = 0.2f * random_.Next(-10, 10);
            s.Velocity.Z = 0.2f * random_.Next(-10, 10);
            s.Velocity.Y = 0.2f * random_.Next(-3, 3);
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
        world = world * Matrix::CreateTranslation(0.0f, 0.0f, -5.0f);

        Matrix shadowMatrix = Matrix::getIdentityProperty();
        shadowMatrix.M22 = 0.0f;

        for (int i = 0; i < numSpheres; i++) {
            Matrix worldX = world * Matrix::CreateScale(spheres_[i].Radius / 0.5f)
                                  * Matrix::CreateTranslation(spheres_[i].Position);
            primitive_->Draw(worldX, view, projection, spheres_[i].SphereColor, false);
            Matrix shadowX = worldX * shadowMatrix;
            shadowX.M42 = -1.0f;
            primitive_->Draw(shadowX, view, projection, Color(0, 0, 0, 255), true);
        }

        DrawTiltIndicator();

        Game::Draw(gameTime);
    }

private:
    Vector3 CurrentAccel() const {
        if (accelerometerStarted_)
            return accelerometer_.getCurrentValueProperty().getAccelerationProperty();
        return simAccel_;
    }

    void HandleInput() {
        lastKeyboardState_    = currentKeyboardState_;
        lastGamePadState_     = currentGamePadState_;
        currentKeyboardState_ = Keyboard::GetState();
        currentGamePadState_  = GamePad::GetState(PlayerIndex::One);

        if (IsPressed(Keys::Escape, Buttons::Back)) Exit();

        if (!accelerometerStarted_) {
            const float step = 0.03f;
            if (currentKeyboardState_.IsKeyDown(Keys::Left))
                simAccel_.Y = MathHelper::Clamp(simAccel_.Y - step, -tiltLimit, tiltLimit);
            if (currentKeyboardState_.IsKeyDown(Keys::Right))
                simAccel_.Y = MathHelper::Clamp(simAccel_.Y + step, -tiltLimit, tiltLimit);
            if (currentKeyboardState_.IsKeyDown(Keys::Up))
                simAccel_.Z = MathHelper::Clamp(simAccel_.Z - step,
                                                 -tiltOffset - tiltLimit,
                                                 -tiltOffset + tiltLimit);
            if (currentKeyboardState_.IsKeyDown(Keys::Down))
                simAccel_.Z = MathHelper::Clamp(simAccel_.Z + step,
                                                 -tiltOffset - tiltLimit,
                                                 -tiltOffset + tiltLimit);
        }
    }

    void UpdateSpheres(GameTime& gameTime) {
        Vector3 gravity = Vector3::UnitY * -4.0f;
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        {
            float shakeForce = 1.0f;
            Vector3 accel = CurrentAccel();
            float newMag = accel.Length();

            if (accelhistory_[1] > 1.3f && accelhistory_[0] < accelhistory_[1] && accelhistory_[1] > newMag)
                shakeForce += 10.0f * (accelhistory_[1] - 1.3f) / 3.5f;
            accelhistory_[0] = accelhistory_[1];
            accelhistory_[1] = newMag;

            float rotateX = std::max(std::min(-(accel.Z + tiltOffset), tiltLimit), -tiltLimit);
            float rotateY = std::max(std::min(accel.Y, tiltLimit), -tiltLimit);
            gravity = Vector3::Transform(gravity, Matrix::CreateRotationX(rotateX * MathHelper::PiOver2));
            gravity = Vector3::Transform(gravity, Matrix::CreateRotationZ(rotateY * MathHelper::PiOver2));

            if (shakeForce > 1.0f) {
                for (int i = 0; i < numSpheres; i++) {
                    Sphere& s = spheres_[i];
                    if (s.Position.Y <= floorPlaneHeight + s.Radius + 0.01f) {
                        s.Velocity.Y += 0.1f;
                        float speed = s.Velocity.Length();
                        float adj = std::min(std::max(speed, 2.0f), 4.0f);
                        if (speed > 0.0f)
                            s.Velocity = s.Velocity * (adj * shakeForce / speed);
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
                        s.Velocity.LengthSquared() < 0.25f)
                        s.Velocity.Y = 0.0f;
                    else
                        s.Velocity.Y = -s.Velocity.Y * collisionDamping;
                }
            }
            auto wallBounce = [&](float& pos, float& vel, float lo, float hi) {
                if (pos < lo) { pos = lo; if (vel < 0) vel = -vel * collisionDamping; }
                if (pos > hi) { pos = hi; if (vel > 0) vel = -vel * collisionDamping; }
            };
            wallBounce(s.Position.X, s.Velocity.X, -worldSize + s.Radius, worldSize - s.Radius);
            wallBounce(s.Position.Z, s.Velocity.Z, -worldSize + s.Radius, worldSize - s.Radius);
        }
    }

    void SphereCollisionImplicit(Sphere& s1, Sphere& s2) {
        const float K_ELASTIC = 0.75f;
        Vector3 rel   = s2.Position - s1.Position;
        float dist2   = rel.LengthSquared();
        float radii   = s1.Radius + s2.Radius;
        if (dist2 >= radii * radii) return;

        float dist    = rel.Length();
        Vector3 unit  = rel * (1.0f / dist);
        Vector3 pen   = unit * (radii - dist);

        float inv  = 1.0f / (s1.Mass + s2.Mass);
        float w1   = s1.Mass * inv, w2 = s2.Mass * inv;
        s1.Position = s1.Position - pen * w2;
        s2.Position = s2.Position + pen * w1;

        Vector3 vTot = s1.Velocity * w1 + s2.Velocity * w2;
        Vector3 i2   = (s2.Velocity - vTot) * s2.Mass;
        if (Vector3::Dot(i2, unit) < 0) {
            Vector3 di = unit * Vector3::Dot(i2, unit);
            i2 = i2 - di * (K_ELASTIC + 1.0f);
            s1.Velocity = (i2 * -1.0f) / s1.Mass + vTot;
            s2.Velocity = i2 / s2.Mass + vTot;
        }
    }

    // Draw a filled rectangle using the 1x1 white pixel texture.
    void FillRect(int x, int y, int w, int h, Color c) {
        Rectangle dest(x, y, w, h);
        spriteBatch_->Draw(*pixel_, Vector2((float)x, (float)y),
                           std::make_optional(Rectangle(0, 0, 1, 1)),
                           c, 0.0f, Vector2(0.0f, 0.0f),
                           Vector2((float)w, (float)h),
                           SpriteEffects::None, 0.0f);
    }

    void DrawTiltIndicator() {
        Vector3 accel = CurrentAccel();

        // Normalised tilt in [-1, 1]: X=left/right, Y=fwd/back
        float nx =  accel.Y / tiltLimit;
        float ny = -(accel.Z + tiltOffset) / tiltLimit;
        nx = std::max(-1.0f, std::min(1.0f, nx));
        ny = std::max(-1.0f, std::min(1.0f, ny));

        // Box in top-left corner
        const int BOX  = 70;   // outer box size
        const int PAD  =  5;   // margin from screen edge
        const int DOT  = 11;   // dot diameter
        const int CX   = PAD + BOX / 2;
        const int CY   = PAD + BOX / 2;
        const int HALF = (BOX - DOT) / 2;

        // Indicator colour: yellow for desktop sim, cyan for real sensor
        Color dotColor = accelerometerStarted_ ? Color(0, 255, 255, 220)
                                               : Color(255, 220, 0, 220);

        spriteBatch_->Begin();

        // Dark background
        FillRect(PAD, PAD, BOX, BOX, Color(30, 30, 30, 180));
        // Crosshair lines
        FillRect(CX - 1,    PAD + 2, 2,        BOX - 4, Color(80, 80, 80, 200));
        FillRect(PAD + 2,   CY - 1,  BOX - 4,  2,       Color(80, 80, 80, 200));
        // Center marker (white)
        FillRect(CX - DOT / 2, CY - DOT / 2, DOT, DOT, Color(180, 180, 180, 200));
        // Tilt dot
        int dx = CX + (int)(nx * HALF) - DOT / 2;
        int dy = CY + (int)(ny * HALF) - DOT / 2;
        FillRect(dx, dy, DOT, DOT, dotColor);

        spriteBatch_->End();
    }

    bool IsPressed(Keys key, Buttons button) {
        return (currentKeyboardState_.IsKeyDown(key) && lastKeyboardState_.IsKeyUp(key)) ||
               (currentGamePadState_.IsButtonDown(button) && lastGamePadState_.IsButtonUp(button));
    }
};

} // namespace Bounce
