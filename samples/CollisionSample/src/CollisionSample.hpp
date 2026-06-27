#pragma once
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "BoundingOrientedBox.hpp"
#include "DebugDraw.hpp"
#include "FrameRateCounter.hpp"
#include "TriangleTest.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Quaternion.hpp"
#include "Microsoft/Xna/Framework/Ray.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"

namespace CollisionSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class CollisionSampleGame : public Microsoft::Xna::Framework::Game {
    static constexpr int FrustumGroupIndex    = 0;
    static constexpr int AABoxGroupIndex      = 1;
    static constexpr int OBoxGroupIndex       = 2;
    static constexpr int SphereGroupIndex     = 3;
    static constexpr int RayGroupIndex        = 4;
    static constexpr int NumGroups            = 5;

    static constexpr int TriIndex             = 0;
    static constexpr int SphereIndex          = 1;
    static constexpr int AABoxIndex           = 2;
    static constexpr int OBoxIndex            = 3;
    static constexpr int NumSecondaryShapes   = 4;

    static constexpr float CAMERA_SPACING  = 50.0f;
    static constexpr float YAW_RATE        = 1.0f;
    static constexpr float PITCH_RATE      = 0.75f;
    static constexpr float DISTANCE_RATE   = 10.0f;

    GraphicsDeviceManager graphics_;
    std::unique_ptr<DebugDraw> debugDraw_;
    std::unique_ptr<FrameRateCounter> frameRateCounter_;

    // Primary shapes
    BoundingFrustum      primaryFrustum_;
    BoundingBox          primaryAABox_;
    BoundingOrientedBox  primaryOBox_;
    BoundingSphere       primarySphere_;
    Ray                  primaryRay_;

    // Secondary shapes
    Triangle            secondaryTris_[NumGroups];
    BoundingSphere      secondarySpheres_[NumGroups];
    BoundingBox         secondaryAABoxes_[NumGroups];
    BoundingOrientedBox secondaryOBoxes_[NumGroups];

    // Collision results
    ContainmentType collideResults_[NumGroups][NumSecondaryShapes]{};
    std::optional<Vector3> rayHitResult_;

    // Camera
    Vector3 cameraOrigins_[NumGroups];
    int   currentCamera_  = 3;
    bool  cameraOrtho_    = false;
    float cameraYaw_      = 0.0f;
    float cameraPitch_    = 0.0f;
    float cameraDistance_ = 20.0f;
    Vector3 cameraTarget_;

    KeyboardState currentKeyboardState_;
    KeyboardState previousKeyboardState_;
    GamePadState  currentGamePadState_;
    GamePadState  previousGamePadState_;

    double unpausedClock_ = 0.0;
    bool   paused_        = false;

    std::unique_ptr<SpriteBatch> helpSpriteBatch_;
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "CollisionSampleGame";
        return name;
    }

    CollisionSampleGame()
        : graphics_(this)
        , primaryFrustum_(Matrix::getIdentityProperty())
        , primarySphere_(Vector3::Zero, 5.0f)
    {
        graphics_.setPreferredBackBufferWidthProperty(853);
        graphics_.setPreferredBackBufferHeightProperty(480);
    }

protected:
    void Initialize() override {
        debugDraw_ = std::make_unique<DebugDraw>(getGraphicsDeviceProperty());

        frameRateCounter_ = std::make_unique<FrameRateCounter>(*this);
        getComponentsProperty().Add(frameRateCounter_.get());

        // Primary frustum
        Matrix m1 = Matrix::CreatePerspectiveFieldOfView(MathHelper::PiOver4, 1.77778f, 0.5f, 10.0f);
        Matrix m2 = Matrix::CreateTranslation(Vector3(0.0f, 0.0f, -7.0f));
        primaryFrustum_ = BoundingFrustum(Matrix::Multiply(m2, m1));
        cameraOrigins_[FrustumGroupIndex] = Vector3::Zero;

        // Primary axis-aligned box
        primaryAABox_.Min = Vector3(CAMERA_SPACING - 3, -4, -5);
        primaryAABox_.Max = Vector3(CAMERA_SPACING + 3,  4,  5);
        cameraOrigins_[AABoxGroupIndex] = Vector3(CAMERA_SPACING, 0.0f, 0.0f);

        // Primary oriented box
        primaryOBox_.Center      = Vector3(-CAMERA_SPACING, 0.0f, 0.0f);
        primaryOBox_.HalfExtent  = Vector3(3.0f, 4.0f, 5.0f);
        primaryOBox_.Orientation = Quaternion::CreateFromYawPitchRoll(0.8f, 0.7f, 0.0f);
        cameraOrigins_[OBoxGroupIndex] = primaryOBox_.Center;

        // Primary sphere
        primarySphere_.Center = Vector3(0.0f, 0.0f, -CAMERA_SPACING);
        primarySphere_.Radius = 5.0f;
        cameraOrigins_[SphereGroupIndex] = primarySphere_.Center;

        // Primary ray
        primaryRay_.Position  = Vector3(0.0f, 0.0f, CAMERA_SPACING);
        primaryRay_.Direction = Vector3::UnitZ;
        cameraOrigins_[RayGroupIndex] = primaryRay_.Position;

        // Secondary shapes defaults
        Vector3 half(0.5f, 0.5f, 0.5f);
        for (int i = 0; i < NumGroups; i++) {
            secondarySpheres_[i]  = BoundingSphere(Vector3::Zero, 1.0f);
            secondaryOBoxes_[i]   = BoundingOrientedBox(Vector3::Zero, half, Quaternion::Identity);
            secondaryAABoxes_[i].Min = -half;
            secondaryAABoxes_[i].Max =  half;
            secondaryTris_[i]     = Triangle();
        }

        rayHitResult_ = std::nullopt;
        helpSpriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        cameraYaw_      = MathHelper::Pi * 0.75f;
        cameraPitch_    = MathHelper::PiOver4;
        cameraDistance_ = 20.0f;
        cameraTarget_   = cameraOrigins_[0];
        paused_         = false;

        Game::Initialize();
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;
        ReadInputDevices();

        if (currentKeyboardState_.IsKeyDown(Keys::Escape) ||
            currentGamePadState_.IsButtonDown(Buttons::Back))
            Exit();

        if (!paused_)
            unpausedClock_ += gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        Animate();
        Collide();
        HandleInput(gameTime);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255)); // CornflowerBlue

        float aspect = getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty();
        float yawCos   = std::cos(cameraYaw_);
        float yawSin   = std::sin(cameraYaw_);
        float pitchCos = std::cos(cameraPitch_);
        float pitchSin = std::sin(cameraPitch_);

        Vector3 eye(
            cameraDistance_ * pitchCos * yawSin + cameraTarget_.X,
            cameraDistance_ * pitchSin + cameraTarget_.Y,
            cameraDistance_ * pitchCos * yawCos + cameraTarget_.Z);

        Matrix view       = Matrix::CreateLookAt(eye, cameraTarget_, Vector3::Up);
        Matrix projection = cameraOrtho_
            ? Matrix::CreateOrthographic(aspect * cameraDistance_, cameraDistance_, 1.0f, 1000.0f)
            : Matrix::CreatePerspectiveFieldOfView(MathHelper::Pi / 4.0f, aspect, 1.0f, 1000.0f);

        debugDraw_->Begin(view, projection);

        // Ground planes
        for (int g = 0; g < NumGroups; ++g) {
            Vector3 origin(cameraOrigins_[g].X - 20, cameraOrigins_[g].Y - 10, cameraOrigins_[g].Z - 20);
            debugDraw_->DrawWireGrid(Vector3::UnitX * 40, Vector3::UnitZ * 40, origin, 20, 20, Color(0,0,0,255));
        }

        DrawPrimaryShapes();

        // Secondary shapes
        for (int g = 0; g < NumGroups; g++) {
            debugDraw_->DrawWireSphere(secondarySpheres_[g], GetCollideColor(g, SphereIndex));
            debugDraw_->DrawWireBox(secondaryAABoxes_[g],    GetCollideColor(g, AABoxIndex));
            debugDraw_->DrawWireBox(secondaryOBoxes_[g],     GetCollideColor(g, OBoxIndex));
            debugDraw_->DrawWireTriangle(secondaryTris_[g],  GetCollideColor(g, TriIndex));
        }

        // Ray hit result
        if (rayHitResult_.has_value()) {
            Vector3 size(0.05f, 0.05f, 0.05f);
            BoundingBox weeBox;
            weeBox.Min = rayHitResult_.value() - size;
            weeBox.Max = rayHitResult_.value() + size;
            debugDraw_->DrawWireBox(weeBox, Color(255, 255, 0, 255));
        }

        debugDraw_->End();

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            helpSpriteBatch_->Begin();
            helpSpriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
            helpSpriteBatch_->End();
        }

        Game::Draw(gameTime);
    }

private:
    void ReadInputDevices() {
        previousKeyboardState_ = currentKeyboardState_;
        previousGamePadState_  = currentGamePadState_;
        currentKeyboardState_  = Keyboard::GetState();
        currentGamePadState_   = GamePad::GetState(PlayerIndex::One);
    }

    void Animate() {
        float t = (float)(unpausedClock_ / 2.0);
        const float xRate = 1.1f, yRate = 3.6f, zRate = 1.9f;
        const float pathSize = 6.0f;
        const float gap      = 0.25f;

        Quaternion orientation = Quaternion::CreateFromYawPitchRoll(t * 0.2f, t * 1.4f, t);

        for (int g = 0; g < NumGroups; g++) {
            secondarySpheres_[g].Center = cameraOrigins_[g];
            secondarySpheres_[g].Center.X += pathSize * std::sin(xRate * t);
            secondarySpheres_[g].Center.Y += pathSize * std::sin(yRate * t);
            secondarySpheres_[g].Center.Z += pathSize * std::sin(zRate * t);

            secondaryOBoxes_[g].Center      = cameraOrigins_[g];
            secondaryOBoxes_[g].Orientation = orientation;
            secondaryOBoxes_[g].Center.X += pathSize * std::sin(xRate * (t - gap));
            secondaryOBoxes_[g].Center.Y += pathSize * std::sin(yRate * (t - gap));
            secondaryOBoxes_[g].Center.Z += pathSize * std::sin(zRate * (t - gap));

            Vector3 boxsize(1.0f, 1.3f, 1.9f);
            secondaryAABoxes_[g].Min = cameraOrigins_[g] - boxsize * 0.5f;
            secondaryAABoxes_[g].Min.X += pathSize * std::sin(xRate * (t - 2 * gap));
            secondaryAABoxes_[g].Min.Y += pathSize * std::sin(yRate * (t - 2 * gap));
            secondaryAABoxes_[g].Min.Z += pathSize * std::sin(zRate * (t - 2 * gap));
            secondaryAABoxes_[g].Max = secondaryAABoxes_[g].Min + boxsize;

            Vector3 trianglePos = cameraOrigins_[g];
            trianglePos.X += pathSize * std::sin(xRate * (t - 3 * gap));
            trianglePos.Y += pathSize * std::sin(yRate * (t - 3 * gap));
            trianglePos.Z += pathSize * std::sin(zRate * (t - 3 * gap));
            secondaryTris_[g].V0 = trianglePos + Vector3::Transform(Vector3(0.0f,    2.0f, 0.0f), orientation);
            secondaryTris_[g].V1 = trianglePos + Vector3::Transform(Vector3(1.73f,  -1.0f, 0.0f), orientation);
            secondaryTris_[g].V2 = trianglePos + Vector3::Transform(Vector3(-1.73f, -1.0f, 0.0f), orientation);
        }

        const float sweepTime = 3.1f;
        float rayDt = (-std::abs(std::fmod(t / sweepTime, 2.0f) - 1.0f) * NumSecondaryShapes + 0.5f) * gap;
        primaryRay_.Direction.X = std::sin(xRate * (t + rayDt));
        primaryRay_.Direction.Y = std::sin(yRate * (t + rayDt));
        primaryRay_.Direction.Z = std::sin(zRate * (t + rayDt));
        primaryRay_.Direction.Normalize();
    }

    void Collide() {
        collideResults_[FrustumGroupIndex][SphereIndex] = primaryFrustum_.Contains(secondarySpheres_[FrustumGroupIndex]);
        collideResults_[FrustumGroupIndex][OBoxIndex]   = BoundingOrientedBox::Contains(primaryFrustum_, secondaryOBoxes_[FrustumGroupIndex]);
        collideResults_[FrustumGroupIndex][AABoxIndex]  = primaryFrustum_.Contains(secondaryAABoxes_[FrustumGroupIndex]);
        collideResults_[FrustumGroupIndex][TriIndex]    = TriangleTest::Contains(primaryFrustum_, secondaryTris_[FrustumGroupIndex]);

        collideResults_[AABoxGroupIndex][SphereIndex]   = primaryAABox_.Contains(secondarySpheres_[AABoxGroupIndex]);
        collideResults_[AABoxGroupIndex][OBoxIndex]     = BoundingOrientedBox::Contains(primaryAABox_, secondaryOBoxes_[AABoxGroupIndex]);
        collideResults_[AABoxGroupIndex][AABoxIndex]    = primaryAABox_.Contains(secondaryAABoxes_[AABoxGroupIndex]);
        collideResults_[AABoxGroupIndex][TriIndex]      = TriangleTest::Contains(primaryAABox_, secondaryTris_[AABoxGroupIndex]);

        collideResults_[OBoxGroupIndex][SphereIndex]    = primaryOBox_.Contains(secondarySpheres_[OBoxGroupIndex]);
        collideResults_[OBoxGroupIndex][OBoxIndex]      = primaryOBox_.Contains(secondaryOBoxes_[OBoxGroupIndex]);
        collideResults_[OBoxGroupIndex][AABoxIndex]     = primaryOBox_.Contains(secondaryAABoxes_[OBoxGroupIndex]);
        collideResults_[OBoxGroupIndex][TriIndex]       = TriangleTest_ContainsOBox(primaryOBox_, secondaryTris_[OBoxGroupIndex]);

        collideResults_[SphereGroupIndex][SphereIndex]  = primarySphere_.Contains(secondarySpheres_[SphereGroupIndex]);
        collideResults_[SphereGroupIndex][OBoxIndex]    = BoundingOrientedBox::Contains(primarySphere_, secondaryOBoxes_[SphereGroupIndex]);
        collideResults_[SphereGroupIndex][AABoxIndex]   = primarySphere_.Contains(secondaryAABoxes_[SphereGroupIndex]);
        collideResults_[SphereGroupIndex][TriIndex]     = TriangleTest::Contains(primarySphere_, secondaryTris_[SphereGroupIndex]);

        for (int g = 0; g < NumGroups; g++)
            for (int s = 0; s < NumSecondaryShapes; s++)
                collideResults_[RayGroupIndex][s] = ContainmentType::Disjoint;
        rayHitResult_ = std::nullopt;

        float dist = -1.0f;
        auto r = primaryRay_.Intersects(secondarySpheres_[RayGroupIndex]);
        if (r.has_value()) { collideResults_[RayGroupIndex][SphereIndex] = ContainmentType::Intersects; dist = r.value(); }

        r = secondaryOBoxes_[RayGroupIndex].Intersects(primaryRay_);
        if (r.has_value()) { collideResults_[RayGroupIndex][OBoxIndex] = ContainmentType::Intersects; dist = r.value(); }

        r = primaryRay_.Intersects(secondaryAABoxes_[RayGroupIndex]);
        if (r.has_value()) { collideResults_[RayGroupIndex][AABoxIndex] = ContainmentType::Intersects; dist = r.value(); }

        r = TriangleTest::Intersects(primaryRay_, secondaryTris_[RayGroupIndex]);
        if (r.has_value()) { collideResults_[RayGroupIndex][TriIndex] = ContainmentType::Intersects; dist = r.value(); }

        if (dist > 0.0f)
            rayHitResult_ = primaryRay_.Position + primaryRay_.Direction * dist;
    }

    void HandleInput(GameTime& gameTime) {
        float dt = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        if (paused_) {
            if (currentKeyboardState_.IsKeyDown(Keys::OemOpenBrackets))
                unpausedClock_ -= dt;
            if (currentKeyboardState_.IsKeyDown(Keys::OemCloseBrackets))
                unpausedClock_ += dt;
        }

        cameraYaw_   += currentGamePadState_.getThumbSticksProperty().getRightProperty().X * dt * YAW_RATE;
        cameraPitch_ += currentGamePadState_.getThumbSticksProperty().getRightProperty().Y * dt * PITCH_RATE;

        if (currentKeyboardState_.IsKeyDown(Keys::Right)) cameraYaw_   += dt * YAW_RATE;
        if (currentKeyboardState_.IsKeyDown(Keys::Left))  cameraYaw_   -= dt * YAW_RATE;
        if (currentKeyboardState_.IsKeyDown(Keys::Up))    cameraPitch_ += dt * PITCH_RATE;
        if (currentKeyboardState_.IsKeyDown(Keys::Down))  cameraPitch_ -= dt * PITCH_RATE;

        if (currentGamePadState_.IsButtonDown(Buttons::LeftTrigger) ||
            currentKeyboardState_.IsKeyDown(Keys::Subtract) ||
            currentKeyboardState_.IsKeyDown(Keys::OemMinus))
            cameraDistance_ += dt * DISTANCE_RATE;

        if (currentGamePadState_.IsButtonDown(Buttons::RightTrigger) ||
            currentKeyboardState_.IsKeyDown(Keys::Add) ||
            currentKeyboardState_.IsKeyDown(Keys::OemPlus))
            cameraDistance_ -= dt * DISTANCE_RATE;

        // Group cycle (G key)
        if ((currentKeyboardState_.IsKeyDown(Keys::G) && previousKeyboardState_.IsKeyUp(Keys::G)) ||
            (currentGamePadState_.IsButtonDown(Buttons::A) && previousGamePadState_.IsButtonUp(Buttons::A)))
            currentCamera_ = (currentCamera_ + 1) % NumGroups;

        // Camera reset (Home)
        if ((currentKeyboardState_.IsKeyDown(Keys::Home) && previousKeyboardState_.IsKeyUp(Keys::Home)) ||
            (currentGamePadState_.IsButtonDown(Buttons::Y) && previousGamePadState_.IsButtonUp(Buttons::Y))) {
            cameraYaw_      = MathHelper::Pi * 0.75f;
            cameraPitch_    = MathHelper::PiOver4;
            cameraDistance_ = 40.0f;
        }

        // Ortho/perspective toggle (B)
        if ((currentKeyboardState_.IsKeyDown(Keys::B) && previousKeyboardState_.IsKeyUp(Keys::B)) ||
            (currentGamePadState_.IsButtonDown(Buttons::B) && previousGamePadState_.IsButtonUp(Buttons::B)))
            cameraOrtho_ = !cameraOrtho_;

        if (currentKeyboardState_.IsKeyDown(Keys::O)) cameraOrtho_ = true;
        if (currentKeyboardState_.IsKeyDown(Keys::P)) cameraOrtho_ = false;

        // Pause (Space)
        if ((currentKeyboardState_.IsKeyDown(Keys::Space) && previousKeyboardState_.IsKeyUp(Keys::Space)) ||
            (currentGamePadState_.IsButtonDown(Buttons::X) && previousGamePadState_.IsButtonUp(Buttons::X)))
            paused_ = !paused_;

        cameraYaw_      = MathHelper::WrapAngle(cameraYaw_);
        cameraPitch_    = MathHelper::Clamp(cameraPitch_, -MathHelper::PiOver2, MathHelper::PiOver2);
        cameraDistance_ = MathHelper::Clamp(cameraDistance_, 2.0f, 80.0f);

        float lerp = std::min(4.0f * dt, 1.0f);
        cameraTarget_ = cameraTarget_ * (1.0f - lerp) + cameraOrigins_[currentCamera_] * lerp;
    }

    void DrawPrimaryShapes() {
        debugDraw_->DrawWireBox(primaryAABox_,  Color(255, 255, 255, 255));
        debugDraw_->DrawWireBox(primaryOBox_,   Color(255, 255, 255, 255));
        debugDraw_->DrawWireFrustum(primaryFrustum_, Color(255, 255, 255, 255));
        debugDraw_->DrawWireSphere(primarySphere_,   Color(255, 255, 255, 255));
        debugDraw_->DrawRay(primaryRay_, Color(255, 0, 0, 255), 10.0f);
    }

    Color GetCollideColor(int group, int shape) {
        switch (collideResults_[group][shape]) {
            case ContainmentType::Contains:   return Color(255,   0,   0, 255); // Red
            case ContainmentType::Intersects: return Color(255, 255,   0, 255); // Yellow
            default:                          return Color(211, 211, 211, 255); // LightGray
        }
    }
};

} // namespace CollisionSample
