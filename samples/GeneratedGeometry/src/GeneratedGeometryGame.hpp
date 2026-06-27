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
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "Terrain.hpp"
#include "Sky.hpp"

namespace GeneratedGeometrySample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;
    using namespace Microsoft::Xna::Framework::Input;

    class GeneratedGeometryGame : public Microsoft::Xna::Framework::Game
    {
        GraphicsDeviceManager graphics;
        Terrain               terrain;
        Sky                   sky;

        std::unique_ptr<SpriteBatch> helpSpriteBatch_;
        std::optional<Texture2D>     helpTexture_;
        float helpTimer_ = 0.0f;
        bool  prevF1_    = false;

    public:
        GeneratedGeometryGame() : graphics(this)
        {
            getContentProperty().setRootDirectoryProperty("Content");
        }

        const std::string& GetTypeName() const override
        {
            static const std::string name = "GeneratedGeometryGame";
            return name;
        }

        void LoadContent() override
        {
            auto& device  = *graphics.getGraphicsDeviceProperty();
            auto& content = getContentProperty();

            terrain.LoadContent(content, device);
            sky.LoadContent(content, device);
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
            KeyboardState kbState = Keyboard::GetState();
            GamePadState  gpState = GamePad::GetState(PlayerIndex::One);

            if (kbState.IsKeyDown(Keys::Escape) ||
                gpState.IsButtonDown(Buttons::Back))
                Exit();

            Game::Update(gameTime);
        }

        void Draw(const GameTime& gameTime) override
        {
            auto& device = *graphics.getGraphicsDeviceProperty();
            device.Clear(Color::Black);

            float time = static_cast<float>(
                gameTime.getTotalGameTimeProperty().getTotalSecondsProperty()) * 0.333f;

            float cameraX = std::cos(time);
            float cameraY = std::sin(time);

            Vector3 cameraPosition = Vector3(cameraX, 0.0f, cameraY) * 64.0f;
            Vector3 cameraFront    = Vector3(-cameraY, 0.0f, cameraX);

            Matrix view = Matrix::CreateLookAt(
                cameraPosition,
                cameraPosition + cameraFront,
                Vector3::Up);

            const auto& vp = device.getViewportProperty();
            float aspect = static_cast<float>(vp.getWidthProperty()) /
                           static_cast<float>(vp.getHeightProperty());
            Matrix projection = Matrix::CreatePerspectiveFieldOfView(
                MathHelper::PiOver4, aspect, 1.0f, 10000.0f);

            terrain.Draw(view, projection);
            sky.Draw(view, projection);

            if (helpTimer_ > 0.0f) {
                int hw = helpTexture_->getWidthProperty();
                int hh = helpTexture_->getHeightProperty();
                float sx = (float)((vp.getWidthProperty()  - hw) / 2);
                float sy = (float)((vp.getHeightProperty() - hh) / 2);
                helpSpriteBatch_->Begin();
                helpSpriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
                helpSpriteBatch_->End();
            }

            Game::Draw(gameTime);
        }
    };

} // namespace GeneratedGeometrySample
