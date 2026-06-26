#pragma once

#include <cmath>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

#include "DebugShapeRenderer.hpp"

namespace ShapeRenderingSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Input;

    class ShapeRenderingGame : public Microsoft::Xna::Framework::Game
    {
        GraphicsDeviceManager graphics;

        BoundingBox     box;
        BoundingFrustum frustum;
        BoundingSphere  sphere;

    public:
        ShapeRenderingGame()
            : graphics(this)
            , frustum(Matrix::getIdentityProperty())
        {
            getContentProperty().setRootDirectoryProperty("Content");
        }

        const std::string& GetTypeName() const override
        {
            static const std::string name = "ShapeRenderingGame";
            return name;
        }

        void LoadContent() override
        {
            auto& device = *graphics.getGraphicsDeviceProperty();

            box = BoundingBox(Vector3(-3.0f, -3.0f, -3.0f), Vector3(3.0f, 3.0f, 3.0f));

            Matrix frustumView = Matrix::CreateLookAt(Vector3::Zero, Vector3::UnitX, Vector3::Up);
            Matrix frustumProjection = Matrix::CreatePerspectiveFieldOfView(
                MathHelper::PiOver4, 16.0f / 9.0f, 1.0f, 5.0f);
            frustum = BoundingFrustum(frustumView * frustumProjection);

            sphere = BoundingSphere(Vector3::Zero, 3.0f);

            DebugShapeRenderer::Initialize(device);
        }

        void Update(GameTime& gameTime) override
        {
            GamePadState  gpState = GamePad::GetState(PlayerIndex::One);
            KeyboardState kbState = Keyboard::GetState();

            if (kbState.IsKeyDown(Keys::Escape) ||
                gpState.IsButtonDown(Buttons::Back))
                Exit();

            Game::Update(gameTime);
        }

        void Draw(const GameTime& gameTime) override
        {
            auto& device = *graphics.getGraphicsDeviceProperty();
            device.Clear(Color::CornflowerBlue);

            float angle = static_cast<float>(
                gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());
            Vector3 eye(std::cos(angle * 0.5f), 0.0f, std::sin(angle * 0.5f));
            eye = eye * 12.0f;
            eye.Y = 5.0f;

            Matrix view = Matrix::CreateLookAt(eye, Vector3::Zero, Vector3::Up);

            const auto& vp = device.getViewportProperty();
            float aspect = static_cast<float>(vp.getWidthProperty()) /
                           static_cast<float>(vp.getHeightProperty());
            Matrix projection = Matrix::CreatePerspectiveFieldOfView(
                MathHelper::PiOver4, aspect, 0.1f, 1000.0f);

            DebugShapeRenderer::AddBoundingBox(box, Color::Yellow);
            DebugShapeRenderer::AddBoundingFrustum(frustum, Color::Green);
            DebugShapeRenderer::AddBoundingSphere(sphere, Color::Red);

            DebugShapeRenderer::AddTriangle(
                Vector3(-1.0f, 0.0f, 0.0f),
                Vector3( 1.0f, 0.0f, 0.0f),
                Vector3( 0.0f, 2.0f, 0.0f),
                Color::Purple);
            DebugShapeRenderer::AddLine(
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(3.0f, 3.0f, 3.0f),
                Color::Brown);

            DebugShapeRenderer::Draw(gameTime, view, projection);

            Game::Draw(gameTime);
        }
    };

} // namespace ShapeRenderingSample
