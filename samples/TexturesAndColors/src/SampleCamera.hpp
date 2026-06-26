#pragma once

#include <cmath>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Quaternion.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

namespace TexturesAndColorsSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Input;

    enum class SampleArcBallCameraMode
    {
        Free            = 0,
        RollConstrained = 1,
    };

    class SampleArcBallCamera
    {
        static constexpr float InputTurnRate = 3.0f;

        Vector3                 target_;
        float                   distance_;
        Quaternion              orientation_;
        float                   inputDistanceRate_ = 4.0f;
        SampleArcBallCameraMode mode_;
        float                   yaw_   = MathHelper::Pi;
        float                   pitch_ = 0.0f;

    public:
        float  Distance          = 1.0f;
        float  InputDistanceRate = 4.0f;
        Vector3 Target           = Vector3::Zero;

        static float ReadKeyboardAxis(const KeyboardState& keyState, Keys downKey, Keys upKey)
        {
            float value = 0.0f;
            if (keyState.IsKeyDown(downKey)) value -= 1.0f;
            if (keyState.IsKeyDown(upKey))   value += 1.0f;
            return value;
        }

        explicit SampleArcBallCamera(SampleArcBallCameraMode controlMode)
            : target_(Vector3::Zero)
            , distance_(1.0f)
            , orientation_(Quaternion::CreateFromAxisAngle(Vector3::Up, MathHelper::Pi))
            , mode_(controlMode)
        {
            Distance          = 1.0f;
            InputDistanceRate = 4.0f;
        }

        Vector3 Direction() const
        {
            Vector3 dir(
                -2.0f * (orientation_.X * orientation_.Z + orientation_.W * orientation_.Y),
                 2.0f * (orientation_.W * orientation_.X - orientation_.Y * orientation_.Z),
                (orientation_.X * orientation_.X + orientation_.Y * orientation_.Y) -
                (orientation_.Z * orientation_.Z + orientation_.W * orientation_.W));
            return Vector3::Normalize(dir);
        }

        Vector3 Right() const
        {
            return Vector3(
                (orientation_.X * orientation_.X + orientation_.W * orientation_.W) -
                (orientation_.Z * orientation_.Z + orientation_.Y * orientation_.Y),
                2.0f * (orientation_.X * orientation_.Y + orientation_.Z * orientation_.W),
                2.0f * (orientation_.X * orientation_.Z - orientation_.Y * orientation_.W));
        }

        Vector3 Up() const
        {
            return Vector3(
                2.0f * (orientation_.X * orientation_.Y - orientation_.Z * orientation_.W),
                (orientation_.Y * orientation_.Y + orientation_.W * orientation_.W) -
                (orientation_.Z * orientation_.Z + orientation_.X * orientation_.X),
                2.0f * (orientation_.Y * orientation_.Z + orientation_.X * orientation_.W));
        }

        Matrix ViewMatrix() const
        {
            return Matrix::CreateLookAt(target_ - Direction() * Distance, target_, Up());
        }

        void OrbitUp(float angle)
        {
            switch (mode_)
            {
                case SampleArcBallCameraMode::Free:
                    orientation_ = orientation_ *
                        Quaternion::CreateFromAxisAngle(Vector3::Right, -angle);
                    orientation_ = Quaternion::Normalize(orientation_);
                    break;
                case SampleArcBallCameraMode::RollConstrained:
                    pitch_ -= angle;
                    pitch_ = MathHelper::Clamp(pitch_,
                        -MathHelper::PiOver2 + 0.0001f,
                         MathHelper::PiOver2 - 0.0001f);
                    orientation_ = Quaternion::CreateFromAxisAngle(Vector3::Up, -yaw_) *
                                   Quaternion::CreateFromAxisAngle(Vector3::Right, pitch_);
                    break;
            }
        }

        void OrbitRight(float angle)
        {
            switch (mode_)
            {
                case SampleArcBallCameraMode::Free:
                    orientation_ = orientation_ *
                        Quaternion::CreateFromAxisAngle(Vector3::Up, angle);
                    orientation_ = Quaternion::Normalize(orientation_);
                    break;
                case SampleArcBallCameraMode::RollConstrained:
                    yaw_ -= angle;
                    yaw_ = std::fmod(yaw_, MathHelper::TwoPi);
                    orientation_ = Quaternion::CreateFromAxisAngle(Vector3::Up, -yaw_) *
                                   Quaternion::CreateFromAxisAngle(Vector3::Right, pitch_);
                    orientation_ = Quaternion::Normalize(orientation_);
                    break;
            }
        }

        void RotateClockwise(float angle)
        {
            if (mode_ == SampleArcBallCameraMode::Free)
            {
                orientation_ = orientation_ *
                    Quaternion::CreateFromAxisAngle(Vector3::Forward, angle);
                orientation_ = Quaternion::Normalize(orientation_);
            }
        }

        void SetCamera(Vector3 position, Vector3 lookAt, Vector3 up)
        {
            Matrix temp = Matrix::Invert(Matrix::CreateLookAt(position, lookAt, up));
            target_      = lookAt;
            orientation_ = Quaternion::CreateFromRotationMatrix(temp);

            if (mode_ != SampleArcBallCameraMode::Free)
            {
                Vector3 dir = Direction();
                dir.Y = 0.0f;
                if (dir.Length() == 0.0f)
                    dir = Vector3::Forward;
                dir.Normalize();

                float signX = (dir.X > 0.0f ? 1.0f : dir.X < 0.0f ? -1.0f : 0.0f);
                yaw_   = static_cast<float>(std::acos(-dir.Z)) * signX;
                pitch_ = -(static_cast<float>(std::acos(
                    Vector3::Dot(Vector3::Up, Direction()))) - MathHelper::PiOver2);
            }
        }

        void Reset()
        {
            orientation_ = Quaternion::CreateFromAxisAngle(Vector3::Up, MathHelper::Pi);
            Distance     = 3.0f;
            target_      = Vector3::Zero;
            yaw_         = MathHelper::Pi;
            pitch_       = 0.0f;
        }

        void HandleDefaultKeyboardControls(const KeyboardState& kbState,
                                           const GameTime& gameTime)
        {
            float dt = static_cast<float>(
                gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

            float dX = dt * ReadKeyboardAxis(kbState, Keys::A, Keys::D) * InputTurnRate;
            float dY = dt * ReadKeyboardAxis(kbState, Keys::S, Keys::W) * InputTurnRate;

            if (dY != 0.0f) OrbitUp(dY);
            if (dX != 0.0f) OrbitRight(dX);

            Distance += ReadKeyboardAxis(kbState, Keys::Z, Keys::X)
                        * InputDistanceRate * dt;
            if (Distance < 0.001f) Distance = 0.001f;

            if (mode_ != SampleArcBallCameraMode::Free)
            {
                float dR = dt * ReadKeyboardAxis(kbState, Keys::Q, Keys::E) * InputTurnRate;
                if (dR != 0.0f) RotateClockwise(dR);
            }
        }

        void HandleDefaultGamepadControls(const GamePadState& gpState,
                                          const GameTime& gameTime)
        {
            if (!gpState.getIsConnectedProperty())
                return;

            float dt = static_cast<float>(
                gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

            float dX = gpState.getThumbSticksProperty().getRightProperty().X * dt * InputTurnRate;
            float dY = gpState.getThumbSticksProperty().getRightProperty().Y * dt * InputTurnRate;
            float dR = gpState.getTriggersProperty().getRightProperty() * dt * InputTurnRate;
            dR      -= gpState.getTriggersProperty().getLeftProperty()  * dt * InputTurnRate;

            if (dY != 0.0f) OrbitUp(dY);
            if (dX != 0.0f) OrbitRight(dX);
            if (dR != 0.0f) RotateClockwise(dR);

            if (gpState.IsButtonDown(Buttons::A))
                Distance -= dt * InputDistanceRate;
            if (gpState.IsButtonDown(Buttons::B))
                Distance += dt * InputDistanceRate;
            if (Distance < 0.001f) Distance = 0.001f;
        }
    };
}
