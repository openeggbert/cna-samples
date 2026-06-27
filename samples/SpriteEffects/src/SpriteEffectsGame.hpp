#pragma once

#include <cmath>
#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Effect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"

namespace SpriteEffectsSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Content;
    using namespace Microsoft::Xna::Framework::Graphics;
    using namespace Microsoft::Xna::Framework::Input;

    enum class DemoEffect
    {
        Desaturate = 0,
        Disappear,
        RefractCat,
        RefractGlacier,
        Normalmap,
        Count_
    };

    class SpriteEffectsGame : public Game
    {
        std::unique_ptr<GraphicsDeviceManager> graphics_;

        KeyboardState lastKeyboardState_;
        GamePadState  lastGamePadState_;
        KeyboardState currentKeyboardState_;
        GamePadState  currentGamePadState_;

        DemoEffect currentEffect_ = DemoEffect::Desaturate;

        std::shared_ptr<Effect> desaturateEffect_;
        std::shared_ptr<Effect> disappearEffect_;
        std::shared_ptr<Effect> normalmapEffect_;
        std::shared_ptr<Effect> refractionEffect_;

        Texture2D catTexture_;
        Texture2D catNormalmapTexture_;
        Texture2D glacierTexture_;
        Texture2D waterfallTexture_;

        std::unique_ptr<SpriteBatch> spriteBatch_;

        std::unique_ptr<SpriteBatch> helpSpriteBatch_;
        std::optional<Texture2D>     helpTexture_;
        float helpTimer_ = 0.0f;
        bool  prevF1_    = false;

    public:
        SpriteEffectsGame()
        {
            graphics_ = std::make_unique<GraphicsDeviceManager>(this);
            getContentProperty().setRootDirectoryProperty("Content");
        }

        const std::string& GetTypeName() const override
        {
            static const std::string name = "SpriteEffectsGame";
            return name;
        }

    protected:
        void LoadContent() override
        {
            desaturateEffect_ = getContentProperty().Load<std::shared_ptr<Effect>>("desaturate");
            disappearEffect_  = getContentProperty().Load<std::shared_ptr<Effect>>("disappear");
            normalmapEffect_  = getContentProperty().Load<std::shared_ptr<Effect>>("normalmap");
            refractionEffect_ = getContentProperty().Load<std::shared_ptr<Effect>>("refraction");

            catTexture_         = getContentProperty().Load<Texture2D>("cat");
            catNormalmapTexture_ = getContentProperty().Load<Texture2D>("cat_normalmap");
            glacierTexture_     = getContentProperty().Load<Texture2D>("glacier");
            waterfallTexture_   = getContentProperty().Load<Texture2D>("waterfall");

            spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
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
            HandleInput();

            if (NextButtonPressed())
            {
                int next = static_cast<int>(currentEffect_) + 1;
                if (next >= static_cast<int>(DemoEffect::Count_))
                    next = 0;
                currentEffect_ = static_cast<DemoEffect>(next);
            }

            Game::Update(gameTime);
        }

        void Draw(const GameTime& gameTime) override
        {
            switch (currentEffect_)
            {
                case DemoEffect::Desaturate:    DrawDesaturate(gameTime);    break;
                case DemoEffect::Disappear:     DrawDisappear(gameTime);     break;
                case DemoEffect::Normalmap:     DrawNormalmap(gameTime);     break;
                case DemoEffect::RefractCat:    DrawRefractCat(gameTime);    break;
                case DemoEffect::RefractGlacier: DrawRefractGlacier(gameTime); break;
                default: break;
            }

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
        // --- effect draw helpers ---

        void DrawDesaturate(const GameTime& gameTime)
        {
            float pulsate = Pulsate(gameTime, 4.0f, 0.0f, 255.0f);

            spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend,
                                nullptr, nullptr, nullptr, desaturateEffect_.get());

            spriteBatch_->Draw(glacierTexture_, QuarterOfScreen(0, 0), Color(255, 255, 255, 0));
            spriteBatch_->Draw(glacierTexture_, QuarterOfScreen(1, 0), Color(255, 255, 255, 64));
            spriteBatch_->Draw(glacierTexture_, QuarterOfScreen(0, 1), Color(255, 255, 255, 255));
            spriteBatch_->Draw(glacierTexture_, QuarterOfScreen(1, 1),
                               Color(255, 255, 255, static_cast<int>(pulsate)));

            spriteBatch_->End();
        }

        void DrawDisappear(const GameTime& gameTime)
        {
            auto& device = getGraphicsDeviceProperty();
            const auto& vp = device.getViewportProperty();

            spriteBatch_->Begin();
            spriteBatch_->Draw(glacierTexture_,
                               Rectangle(0, 0, vp.getWidthProperty(), vp.getHeightProperty()),
                               Color::White);
            spriteBatch_->End();

            Vector2 overlayScroll = MoveInCircle(gameTime, 0.8f) * 0.25f;
            if (auto* se = dynamic_cast<ShaderEffect*>(disappearEffect_.get()))
                se->SetUniformVec2("OverlayScroll", overlayScroll.X, overlayScroll.Y);

            device.getTexturesProperty()(1, &waterfallTexture_);

            spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend,
                                nullptr, nullptr, nullptr, disappearEffect_.get());

            int fade = static_cast<int>(Pulsate(gameTime, 2.0f, 0.0f, 255.0f));
            spriteBatch_->Draw(catTexture_,
                               MoveInCirclePos(gameTime, catTexture_, 1.0f),
                               Color(255, 255, 255, fade));

            spriteBatch_->End();
        }

        void DrawRefractCat(const GameTime& gameTime)
        {
            auto& device = getGraphicsDeviceProperty();
            const auto& vp = device.getViewportProperty();

            spriteBatch_->Begin();
            spriteBatch_->Draw(glacierTexture_,
                               Rectangle(0, 0, vp.getWidthProperty(), vp.getHeightProperty()),
                               Color::White);
            spriteBatch_->End();

            Vector2 dispScroll = MoveInCircle(gameTime, 0.1f);
            if (auto* se = dynamic_cast<ShaderEffect*>(refractionEffect_.get()))
                se->SetUniformVec2("DisplacementScroll", dispScroll.X, dispScroll.Y);

            device.getTexturesProperty()(1, &waterfallTexture_);

            spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend,
                                nullptr, nullptr, nullptr, refractionEffect_.get());

            spriteBatch_->Draw(catTexture_,
                               MoveInCirclePos(gameTime, catTexture_, 1.0f),
                               Color::White);

            spriteBatch_->End();
        }

        void DrawRefractGlacier(const GameTime& gameTime)
        {
            auto& device = getGraphicsDeviceProperty();
            const auto& vp = device.getViewportProperty();

            Vector2 dispScroll = MoveInCircle(gameTime, 0.2f);
            if (auto* se = dynamic_cast<ShaderEffect*>(refractionEffect_.get()))
                se->SetUniformVec2("DisplacementScroll", dispScroll.X, dispScroll.Y);

            device.getTexturesProperty()(1, &waterfallTexture_);

            spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend,
                                nullptr, nullptr, nullptr, refractionEffect_.get());

            Rectangle croppedGlacier(32, 32,
                                     glacierTexture_.getWidthProperty()  - 64,
                                     glacierTexture_.getHeightProperty() - 64);

            spriteBatch_->Draw(glacierTexture_,
                               Rectangle(0, 0, vp.getWidthProperty(), vp.getHeightProperty()),
                               croppedGlacier,
                               Color::White);

            spriteBatch_->End();
        }

        void DrawNormalmap(const GameTime& gameTime)
        {
            auto& device = getGraphicsDeviceProperty();
            const auto& vp = device.getViewportProperty();

            spriteBatch_->Begin();
            spriteBatch_->Draw(glacierTexture_,
                               Rectangle(0, 0, vp.getWidthProperty(), vp.getHeightProperty()),
                               Color::White);
            spriteBatch_->End();

            Vector2 spinningLight = MoveInCircle(gameTime, 1.5f);
            float time = static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());
            float tiltUpAndDown = 0.5f + std::cos(time * 0.75f) * 0.1f;
            Vector3 lightDirection(spinningLight.X * tiltUpAndDown,
                                   spinningLight.Y * tiltUpAndDown,
                                   1.0f - tiltUpAndDown);
            lightDirection.Normalize();

            if (auto* se = dynamic_cast<ShaderEffect*>(normalmapEffect_.get()))
                se->SetUniformVec3("LightDirection",
                                   lightDirection.X, lightDirection.Y, lightDirection.Z);

            device.getTexturesProperty()(1, &catNormalmapTexture_);

            spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend,
                                nullptr, nullptr, nullptr, normalmapEffect_.get());

            spriteBatch_->Draw(catTexture_, CenterOnScreen(catTexture_), Color::White);

            spriteBatch_->End();
        }

        // --- helper functions ---

        static float Pulsate(const GameTime& gameTime, float speed, float min, float max)
        {
            float time = static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty()) * speed;
            return min + (std::sin(time) + 1.0f) / 2.0f * (max - min);
        }

        static Vector2 MoveInCircle(const GameTime& gameTime, float speed)
        {
            float time = static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty()) * speed;
            return Vector2(std::cos(time), std::sin(time));
        }

        Vector2 MoveInCirclePos(const GameTime& gameTime, const Texture2D& texture, float speed)
        {
            const auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float x = static_cast<float>(vp.getWidthProperty()  - texture.getWidthProperty())  / 2.0f;
            float y = static_cast<float>(vp.getHeightProperty() - texture.getHeightProperty()) / 2.0f;
            return MoveInCircle(gameTime, speed) * 128.0f + Vector2(x, y);
        }

        Rectangle QuarterOfScreen(int x, int y)
        {
            const auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            int w = vp.getWidthProperty()  / 2;
            int h = vp.getHeightProperty() / 2;
            return Rectangle(w * x, h * y, w, h);
        }

        Vector2 CenterOnScreen(const Texture2D& texture)
        {
            const auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            int x = (vp.getWidthProperty()  - texture.getWidthProperty())  / 2;
            int y = (vp.getHeightProperty() - texture.getHeightProperty()) / 2;
            return Vector2(static_cast<float>(x), static_cast<float>(y));
        }

        void HandleInput()
        {
            lastKeyboardState_ = currentKeyboardState_;
            lastGamePadState_  = currentGamePadState_;

            currentKeyboardState_ = Keyboard::GetState();
            currentGamePadState_  = GamePad::GetState(PlayerIndex::One);

            if (currentKeyboardState_.IsKeyDown(Keys::Escape) ||
                currentGamePadState_.IsButtonDown(Buttons::Back))
            {
                Exit();
            }
        }

        bool NextButtonPressed()
        {
            if (currentKeyboardState_.IsKeyDown(Keys::Space) &&
                !lastKeyboardState_.IsKeyDown(Keys::Space))
                return true;

            if (currentGamePadState_.IsButtonDown(Buttons::A) &&
                lastGamePadState_.IsButtonUp(Buttons::A))
                return true;

            return false;
        }
    };

} // namespace SpriteEffectsSample
