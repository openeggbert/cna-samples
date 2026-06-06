#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cmath>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"

#include "GeometricPrimitive.hpp"
#include "CubePrimitive.hpp"
#include "SpherePrimitive.hpp"
#include "CylinderPrimitive.hpp"
#include "TorusPrimitive.hpp"

namespace Primitives3D
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;
    using namespace Microsoft::Xna::Framework::Input;

    class Primitives3DGame : public Microsoft::Xna::Framework::Game
    {
        GraphicsDeviceManager graphics;

        std::unique_ptr<SpriteBatch> spriteBatch;

        KeyboardState currentKeyboardState;
        KeyboardState lastKeyboardState;
        GamePadState  currentGamePadState;
        GamePadState  lastGamePadState;
        MouseState    currentMouseState;
        MouseState    lastMouseState;

        std::vector<std::unique_ptr<GeometricPrimitive>> primitives;
        int currentPrimitiveIndex = 0;

        RasterizerState wireFrameState;

        std::vector<Color> colors = {
            Color::Red, Color::Green, Color::Blue, Color::White, Color::Black,
        };
        int currentColorIndex = 0;

        bool isWireframe = false;

        bool IsPressed(Keys key, Buttons button) const
        {
            return (currentKeyboardState.IsKeyDown(key) && lastKeyboardState.IsKeyUp(key)) ||
                   (currentGamePadState.IsButtonDown(button) && lastGamePadState.IsButtonUp(button));
        }

        bool LeftMouseIsPressed(Rectangle rect) const
        {
            return currentMouseState.getLeftButtonProperty() == ButtonState::Pressed &&
                   lastMouseState.getLeftButtonProperty()    != ButtonState::Pressed &&
                   rect.Contains(currentMouseState.getXProperty(), currentMouseState.getYProperty());
        }

        void HandleInput()
        {
            lastKeyboardState = currentKeyboardState;
            lastGamePadState  = currentGamePadState;
            lastMouseState    = currentMouseState;

            currentKeyboardState = Keyboard::GetState();
            currentGamePadState  = GamePad::GetState(PlayerIndex::One);
            currentMouseState    = Mouse::GetState();

            if (IsPressed(Keys::Escape, Buttons::Back))
                Exit();

            const auto& vp = graphics.getGraphicsDeviceProperty()->getViewportProperty();
            int halfW = vp.getWidthProperty()  / 2;
            int halfH = vp.getHeightProperty() / 2;
            int w     = vp.getWidthProperty();
            int h     = vp.getHeightProperty();

            Rectangle topScreen(0, 0, w, halfH);
            if (IsPressed(Keys::A, Buttons::A) || LeftMouseIsPressed(topScreen))
                currentPrimitiveIndex = (currentPrimitiveIndex + 1) % static_cast<int>(primitives.size());

            Rectangle botLeft(0, halfH, halfW, halfH);
            if (IsPressed(Keys::B, Buttons::B) || LeftMouseIsPressed(botLeft))
                currentColorIndex = (currentColorIndex + 1) % static_cast<int>(colors.size());

            Rectangle botRight(halfW, halfH, halfW, halfH);
            if (IsPressed(Keys::Y, Buttons::Y) || LeftMouseIsPressed(botRight))
                isWireframe = !isWireframe;
        }

    public:
        Primitives3DGame() : graphics(this)
        {
            getContentProperty().setRootDirectoryProperty("Content");

            wireFrameState.setCullModeProperty(CullMode::None);
            wireFrameState.setFillModeProperty(FillMode::WireFrame);
        }

        const std::string& GetTypeName() const override
        {
            static const std::string name = "Primitives3DGame";
            return name;
        }

        void LoadContent() override
        {
            auto& device = *graphics.getGraphicsDeviceProperty();
            spriteBatch = std::make_unique<SpriteBatch>(device);

            primitives.push_back(std::make_unique<CubePrimitive>(device));
            primitives.push_back(std::make_unique<SpherePrimitive>(device));
            primitives.push_back(std::make_unique<CylinderPrimitive>(device));
            primitives.push_back(std::make_unique<TorusPrimitive>(device));
        }

        void Update(GameTime& gameTime) override
        {
            HandleInput();
            Game::Update(gameTime);
        }

        void Draw(const GameTime& gameTime) override
        {
            auto& device = *graphics.getGraphicsDeviceProperty();
            device.Clear(Color::CornflowerBlue);

            // Wireframe toggle: CNA doesn't yet have GraphicsDevice.RasterizerState
            // XNA-style property — setting is handled inside GeometricPrimitive for now.
            // TODO: apply wireFrameState via GraphicsDevice.RasterizerState once CNA
            //       exposes that setter.

            float time  = static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());
            float yaw   = time * 0.4f;
            float pitch = time * 0.7f;
            float roll  = time * 1.1f;

            Vector3 cameraPos(0.0f, 0.0f, 2.5f);
            const auto& vp = device.getViewportProperty();
            float aspect = static_cast<float>(vp.getWidthProperty()) /
                           static_cast<float>(vp.getHeightProperty());

            Matrix world      = Matrix::CreateFromYawPitchRoll(yaw, pitch, roll);
            Matrix view       = Matrix::CreateLookAt(cameraPos, Vector3::Zero, Vector3::Up);
            Matrix projection = Matrix::CreatePerspectiveFieldOfView(1.0f, aspect, 1.0f, 10.0f);

            primitives[currentPrimitiveIndex]->Draw(world, view, projection, colors[currentColorIndex]);

            Game::Draw(gameTime);
        }
    };
}
