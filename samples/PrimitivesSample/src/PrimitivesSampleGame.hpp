#pragma once

#include <vector>
#include <cmath>
#include <memory>
#include <optional>
#include <string>

#include "System/Random.hpp"

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "PrimitiveBatch.hpp"

namespace PrimitivesSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;
    using namespace Microsoft::Xna::Framework::Input;

    class PrimitivesSampleGame : public Microsoft::Xna::Framework::Game
    {
        static constexpr int   NumStars              = 500;
        static constexpr float PercentBigStars       = 0.2f;
        static constexpr int   MinimumStarBrightness = 56;
        static constexpr int   MaximumStarBrightness = 255;
        static constexpr float ShipSizeX             = 10.0f;
        static constexpr float ShipSizeY             = 15.0f;
        static constexpr float ShipCutoutSize        = 5.0f;
        static constexpr float SunSize               = 30.0f;

        GraphicsDeviceManager graphics;
        PrimitiveBatch* primitiveBatch = nullptr;

        std::unique_ptr<SpriteBatch> helpSpriteBatch_;
        std::optional<Texture2D>     helpTexture_;
        float helpTimer_ = 0.0f;
        bool  prevF1_    = false;

        std::vector<Vector2> stars;
        std::vector<Color>   starColors;

        void CreateStars()
        {
            System::Random random;
            const auto& vp         = graphics.getGraphicsDeviceProperty()->getViewportProperty();
            const int screenWidth  = vp.getWidthProperty();
            const int screenHeight = vp.getHeightProperty();

            for (int i = 0; i < NumStars; ++i)
            {
                Vector2 where(static_cast<float>(random.Next(0, screenWidth)),
                              static_cast<float>(random.Next(0, screenHeight)));

                int greyInt = random.Next(MinimumStarBrightness, MaximumStarBrightness);
                Color color(greyInt, greyInt, greyInt, 255);

                if (static_cast<float>(random.NextDouble()) > PercentBigStars)
                {
                    starColors.push_back(color);
                    stars.push_back(where);
                }
                else
                {
                    for (int j = 0; j < 4; ++j)
                        starColors.push_back(color);

                    stars.push_back(where);
                    stars.push_back(where + Vector2::UnitX);
                    stars.push_back(where + Vector2::UnitY);
                    stars.push_back(where + Vector2::One);
                }
            }
        }

        void DrawStars()
        {
            primitiveBatch->Begin(PrimitiveType::TriangleList);
            for (size_t i = 0; i < stars.size(); ++i)
            {
                primitiveBatch->AddVertex(stars[i],                  starColors[i]);
                primitiveBatch->AddVertex(stars[i] + Vector2::UnitX, starColors[i]);
                primitiveBatch->AddVertex(stars[i] + Vector2::UnitY, starColors[i]);
            }
            primitiveBatch->End();
        }

        void DrawShip(Vector2 where)
        {
            primitiveBatch->Begin(PrimitiveType::LineList);

            primitiveBatch->AddVertex(where + Vector2(0.0f,         -ShipSizeY),              Color::White);
            primitiveBatch->AddVertex(where + Vector2(-ShipSizeX,    ShipSizeY),              Color::White);

            primitiveBatch->AddVertex(where + Vector2(-ShipSizeX,    ShipSizeY),              Color::White);
            primitiveBatch->AddVertex(where + Vector2(0.0f,          ShipSizeY - ShipCutoutSize), Color::White);

            primitiveBatch->AddVertex(where + Vector2(0.0f,          ShipSizeY - ShipCutoutSize), Color::White);
            primitiveBatch->AddVertex(where + Vector2(ShipSizeX,     ShipSizeY),              Color::White);

            primitiveBatch->AddVertex(where + Vector2(ShipSizeX,     ShipSizeY),              Color::White);
            primitiveBatch->AddVertex(where + Vector2(0.0f,         -ShipSizeY),              Color::White);

            primitiveBatch->End();
        }

        void DrawSun(Vector2 where)
        {
            primitiveBatch->Begin(PrimitiveType::LineList);

            primitiveBatch->AddVertex(where + Vector2(0.0f,   SunSize),  Color::White);
            primitiveBatch->AddVertex(where + Vector2(0.0f,  -SunSize),  Color::White);

            primitiveBatch->AddVertex(where + Vector2( SunSize, 0.0f),   Color::White);
            primitiveBatch->AddVertex(where + Vector2(-SunSize, 0.0f),   Color::White);

            float diagonal = std::cos(MathHelper::PiOver4) * SunSize;

            primitiveBatch->AddVertex(where + Vector2(-diagonal,  diagonal), Color::Gray);
            primitiveBatch->AddVertex(where + Vector2( diagonal, -diagonal), Color::Gray);

            primitiveBatch->AddVertex(where + Vector2( diagonal,  diagonal), Color::Gray);
            primitiveBatch->AddVertex(where + Vector2(-diagonal, -diagonal), Color::Gray);

            primitiveBatch->End();
        }

    public:
        PrimitivesSampleGame() : graphics(this)
        {
            getContentProperty().setRootDirectoryProperty("Content");
            graphics.setPreferredBackBufferWidthProperty(853);
            graphics.setPreferredBackBufferHeightProperty(480);
        }

        ~PrimitivesSampleGame() override
        {
            delete primitiveBatch;
        }

        const std::string& GetTypeName() const override
        {
            static const std::string name = "PrimitivesSampleGame";
            return name;
        }

        void Initialize() override
        {
            Game::Initialize();
            CreateStars();
        }

        void LoadContent() override
        {
            primitiveBatch = new PrimitiveBatch(*graphics.getGraphicsDeviceProperty());
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
            if (Keyboard::GetState().IsKeyDown(Keys::Escape) ||
                GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back))
            {
                Exit();
            }
            Game::Update(gameTime);
        }

        void Draw(const GameTime& gameTime) override
        {
            graphics.getGraphicsDeviceProperty()->Clear(Color::Black);

            const auto& vp        = graphics.getGraphicsDeviceProperty()->getViewportProperty();
            const int screenWidth  = vp.getWidthProperty();
            const int screenHeight = vp.getHeightProperty();

            DrawSun(Vector2(screenWidth / 2.0f, screenHeight / 2.0f));
            DrawShip(Vector2(100.0f, screenHeight / 2.0f));
            DrawShip(Vector2(static_cast<float>(screenWidth) - 100.0f, screenHeight / 2.0f));
            DrawStars();

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
}
