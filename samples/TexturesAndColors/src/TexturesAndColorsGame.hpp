#pragma once

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

#include "GeometricPrimitive.hpp"
#include "CubePrimitive.hpp"
#include "SpherePrimitive.hpp"
#include "CylinderPrimitive.hpp"
#include "TorusPrimitive.hpp"
#include "SampleCamera.hpp"
#include "SampleGrid.hpp"

namespace TexturesAndColorsSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;
    using namespace Microsoft::Xna::Framework::Input;
    using namespace Primitives3D;

    class TexturesAndColorsGame : public Microsoft::Xna::Framework::Game
    {
        GraphicsDeviceManager graphics;

        SampleArcBallCamera camera{ SampleArcBallCameraMode::RollConstrained };
        SampleGrid          grid;

        std::vector<std::unique_ptr<GeometricPrimitive>> sampleMeshes;
        int activeMesh      = 0;
        int activeTechnique = 0;

        Matrix world      = Matrix::getIdentityProperty();
        Matrix view       = Matrix::getIdentityProperty();
        Matrix projection = Matrix::getIdentityProperty();

        Vector3 diffuseLightDirection;
        Vector3 diffuseLightColor;
        Vector3 ambientLightColor;

        GamePadState  lastGamePadState;
        KeyboardState lastKeyboardState;

        std::unique_ptr<SpriteBatch> helpSpriteBatch_;
        std::optional<Texture2D>     helpTexture_;
        float helpTimer_ = 0.0f;
        bool  prevF1_    = false;

        void HandleInput(const GameTime& gameTime,
                         const GamePadState& gpState,
                         const KeyboardState& kbState)
        {
            float dt = static_cast<float>(
                gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

            // cycle active mesh (Tab)
            if (kbState.IsKeyDown(Keys::Tab) && lastKeyboardState.IsKeyUp(Keys::Tab))
                activeMesh = (activeMesh + 1) % static_cast<int>(sampleMeshes.size());

            // cycle active technique (Space) — no visible effect without custom HLSL Effect
            if (kbState.IsKeyDown(Keys::Space) && lastKeyboardState.IsKeyUp(Keys::Space))
                activeTechnique = (activeTechnique + 1) % 11;

            // rotate world with arrow keys / left thumbstick
            float dx = SampleArcBallCamera::ReadKeyboardAxis(kbState, Keys::Left, Keys::Right)
                     + gpState.getThumbSticksProperty().getLeftProperty().X;
            float dy = SampleArcBallCamera::ReadKeyboardAxis(kbState, Keys::Down, Keys::Up)
                     + gpState.getThumbSticksProperty().getLeftProperty().Y;

            if (dx != 0.0f)
                world = world * Matrix::CreateFromAxisAngle(camera.Up(), dt * dx);
            if (dy != 0.0f)
                world = world * Matrix::CreateFromAxisAngle(camera.Right(), dt * -dy);
        }

    public:
        TexturesAndColorsGame() : graphics(this)
        {
            getContentProperty().setRootDirectoryProperty("Content");
        }

        const std::string& GetTypeName() const override
        {
            static const std::string name = "TexturesAndColorsGame";
            return name;
        }

        void LoadContent() override
        {
            auto& device = *graphics.getGraphicsDeviceProperty();

            grid.GridColor = Color::LimeGreen;
            grid.GridScale = 1.0f;
            grid.GridSize  = 32;
            grid.LoadGraphicsContent(device);

            camera.Distance = 3.0f;
            camera.OrbitRight(MathHelper::Pi);
            camera.OrbitUp(0.2f);

            // Stand-ins for Content.Load<Model>: Cube, SphereHigh, SphereLow, Cylinder, Torus
            sampleMeshes.push_back(std::make_unique<CubePrimitive>(device));
            sampleMeshes.push_back(std::make_unique<SpherePrimitive>(device, 1.0f, 16));
            sampleMeshes.push_back(std::make_unique<SpherePrimitive>(device, 1.0f,  8));
            sampleMeshes.push_back(std::make_unique<CylinderPrimitive>(device));
            sampleMeshes.push_back(std::make_unique<TorusPrimitive>(device));

            const auto& vp = device.getViewportProperty();
            float w = static_cast<float>(vp.getWidthProperty());
            float h = static_cast<float>(vp.getHeightProperty());
            float aspect = w / h;
            float fov    = MathHelper::PiOver4 * aspect * 3.0f / 4.0f;
            projection = Matrix::CreatePerspectiveFieldOfView(fov, aspect, 0.1f, 1000.0f);

            grid.ProjectionMatrix = projection;
            grid.WorldMatrix      = Matrix::getIdentityProperty();
            helpSpriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
            helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
        }

        void Update(GameTime& gameTime) override
        {
            float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
            bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
            if (curF1 && !prevF1_) helpTimer_ = 10.0f;
            prevF1_ = curF1;
            if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;
            GamePadState  gpState = GamePad::GetState(PlayerIndex::One);
            KeyboardState kbState = Keyboard::GetState();

            if (kbState.IsKeyDown(Keys::Escape) ||
                gpState.IsButtonDown(Buttons::Back))
                Exit();

            camera.HandleDefaultGamepadControls(gpState, gameTime);
            camera.HandleDefaultKeyboardControls(kbState, gameTime);
            HandleInput(gameTime, gpState, kbState);

            diffuseLightDirection = Vector3(-1.0f, -1.0f, -1.0f);
            diffuseLightDirection.Normalize();

            diffuseLightColor = Vector3(
                Color::CornflowerBlue.getRProperty() / 255.0f,
                Color::CornflowerBlue.getGProperty() / 255.0f,
                Color::CornflowerBlue.getBProperty() / 255.0f);

            ambientLightColor = Vector3(
                Color::DarkSlateGray.getRProperty() / 255.0f,
                Color::DarkSlateGray.getGProperty() / 255.0f,
                Color::DarkSlateGray.getBProperty() / 255.0f);

            view = camera.ViewMatrix();

            grid.ViewMatrix = view;

            lastGamePadState  = gpState;
            lastKeyboardState = kbState;

            Game::Update(gameTime);
        }

        void Draw(const GameTime& gameTime) override
        {
            auto& device = *graphics.getGraphicsDeviceProperty();
            device.Clear(Color::Black);
            device.setDepthStencilStateProperty(DepthStencilState::Default);

            grid.Draw();

            sampleMeshes[activeMesh]->Draw(world, view, projection, Color::White);

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
    };
}
