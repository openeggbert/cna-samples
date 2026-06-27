#pragma once

#include <cmath>
#include <memory>
#include <string>

#include "SpriteSheetHelper.hpp"

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"

namespace SpriteSheetSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Content;
    using namespace Microsoft::Xna::Framework::Graphics;
    using namespace Microsoft::Xna::Framework::Input;

    class SpriteSheetGame : public Game
    {
        std::unique_ptr<GraphicsDeviceManager> graphics_;
        std::unique_ptr<SpriteBatch>           spriteBatch_;

        SpriteSheet spriteSheet_;
        Texture2D   checker_;

        std::unique_ptr<SpriteBatch> helpSpriteBatch_;
        std::optional<Texture2D>     helpTexture_;
        float helpTimer_ = 0.0f;
        bool  prevF1_    = false;

    public:
        SpriteSheetGame()
        {
            graphics_ = std::make_unique<GraphicsDeviceManager>(this);
            graphics_->setPreferredBackBufferWidthProperty(853);
            graphics_->setPreferredBackBufferHeightProperty(480);
            getContentProperty().setRootDirectoryProperty("Content");
        }

        const std::string& GetTypeName() const override
        {
            static const std::string name = "SpriteSheetGame";
            return name;
        }

    protected:
        void LoadContent() override
        {
            spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

            // Build the sprite sheet atlas at runtime from individual source images.
            // (XNA does this at Content Pipeline build time via SpriteSheetProcessor.)
            spriteSheet_.Build(getContentProperty(), getGraphicsDeviceProperty(),
                               {"cat.tga", "glow1.png", "glow2.png", "glow3.png",
                                "glow4.png", "glow5.png", "glow6.png", "glow7.png"});

            checker_ = getContentProperty().Load<Texture2D>("Checker");
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
                Exit();

            Game::Update(gameTime);
        }

        void Draw(const GameTime& gameTime) override
        {
            float time = static_cast<float>(
                gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());

            getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

            spriteBatch_->Begin();

            // Spinning cat sprite (looked up from the sheet by name).
            spriteBatch_->Draw(spriteSheet_.getTextureProperty(),
                               Vector2(200.0f, 250.0f),
                               std::optional<Rectangle>(spriteSheet_.SourceRectangle("cat")),
                               Color::White,
                               time,
                               Vector2(50.0f, 50.0f),
                               1.0f,
                               SpriteEffects::None,
                               0.0f);

            // Animating glow effect — cycles through 7 frames at 20 fps.
            static constexpr int animationFramesPerSecond = 20;
            static constexpr int animationFrameCount      = 7;

            int glowIndex = spriteSheet_.GetIndex("glow1");
            glowIndex += static_cast<int>(time * animationFramesPerSecond) % animationFrameCount;

            spriteBatch_->Draw(spriteSheet_.getTextureProperty(),
                               Rectangle(100, 150, 200, 200),
                               spriteSheet_.SourceRectangle(glowIndex),
                               Color::White);

            spriteBatch_->End();

            DrawEntireSpriteSheetTexture();

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
        void DrawEntireSpriteSheetTexture()
        {
            Vector2 location(500.0f, 80.0f);

            int w = spriteSheet_.getTextureProperty().getWidthProperty();
            int h = spriteSheet_.getTextureProperty().getHeightProperty();

            Rectangle rect(static_cast<int>(location.X),
                           static_cast<int>(location.Y) + 70,
                           w, h);

            // Tiled checkerboard background using LinearWrap sampler.
            spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend,
                                const_cast<SamplerState*>(&SamplerState::LinearWrap),
                                nullptr, nullptr);

            spriteBatch_->Draw(checker_, rect, Rectangle(0, 0, w, h), Color::White);

            spriteBatch_->End();

            // Sprite sheet texture drawn on top.
            // (DrawString omitted — SpriteFont not yet supported by CNA.)
            spriteBatch_->Begin();
            spriteBatch_->Draw(spriteSheet_.getTextureProperty(), rect, Color::White);
            spriteBatch_->End();
        }
    };

} // namespace SpriteSheetSample
