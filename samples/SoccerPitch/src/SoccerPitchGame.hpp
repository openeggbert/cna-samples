#pragma once

// SoccerPitchGame.hpp — C++ port of SoccerPitchGame.cs (XNA 4.0 SoccerPitch
// sample). Demonstrates DualTextureEffect-based detail-texturing of a
// procedurally generated pitch, multipass line rendering (alpha-blend vs.
// alpha-test), and a simple depth-biased shadow.
//
// Adaptation notes (see missing.md for the full write-up):
// - PlanePrimitiveDualTextured uses a single shared UV/tiling factor instead
//   of the original's two independent UV channels — CNA's DualTextureEffect
//   shader only supports one shared UV for both textures.
// - The original's unused `PlanePrimitive` (untextured, VertexPositionNormal)
//   class is not ported — nothing in SoccerPitchGame.cs references it.
// - Touch input is supplemented with a mouse-click fallback (this desktop has
//   no touchscreen), matching the project's established pattern.

#include <algorithm>
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
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/AlphaTestEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/DualTextureEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"

#include "FrameRateCounter.hpp"
#include "Primitives/PlanePrimitiveDualTextured.hpp"
#include "Primitives/PlanePrimitiveTextured.hpp"
#include "Primitives/SpherePrimitiveTextured.hpp"

namespace SoccerPitch {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::AlphaTestEffect;
using Microsoft::Xna::Framework::Graphics::BasicEffect;
using Microsoft::Xna::Framework::Graphics::DualTextureEffect;
using Microsoft::Xna::Framework::Graphics::RasterizerState;
using Microsoft::Xna::Framework::Graphics::SamplerState;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

using FrameRateCounterComponent::FrameRateCounter;

// Demonstrates DualTextureEffect detail-mapping of a procedurally generated
// soccer pitch, multipass alpha-blend/alpha-test line rendering, and a simple
// depth-biased shadow. Port of SoccerPitchGame.cs.
class SoccerPitchGame : public Game {
public:
    SoccerPitchGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");

        getComponentsProperty().Add(new FrameRateCounter(*this));

        // Ensure the framework runs as fast as possible.
        setIsFixedTimeStepProperty(false);
        graphics_.setSynchronizeWithVerticalRetraceProperty(false);
        graphics_.setPreferredBackBufferWidthProperty(PreferredWidth);
        graphics_.setPreferredBackBufferHeightProperty(PreferredHeight);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "SoccerPitchGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        font_.emplace(getContentProperty().Load<SpriteFont>("Font"));

        pitchBasicEffect_.emplace(getGraphicsDeviceProperty());
        pitchBasicEffect_->setLightingEnabledProperty(false);
        pitchBasicEffect_->setPreferPerPixelLightingProperty(false);
        pitchBasicEffect_->setFogEnabledProperty(false);
        pitchBasicEffect_->VertexColorEnabled = false;

        // With a tiling stripe texture, we could tile in any direction; since
        // DualTextureEffect only has one shared UV here (see missing.md), the
        // pitch uses a single tiling factor for both textures.
        Vector2 tiling(PlaneTiling, PlaneTiling);

        pitchPrimitive_.emplace(getGraphicsDeviceProperty(), PlaneSize, tiling);
        pitchStripePrimitive_.emplace(getGraphicsDeviceProperty(), PlaneSize);
        pitchDualTextureEffect_.emplace(getGraphicsDeviceProperty());
        pitchStripeEffect_.emplace(getGraphicsDeviceProperty());
        spherePrimitive_.emplace(getGraphicsDeviceProperty(), SoccerballDiameter, 6);

        pitchBaseTexture_.emplace(getContentProperty().Load<Texture2D>("Base"));
        pitchDetailTexture_.emplace(getContentProperty().Load<Texture2D>("Detail"));
        pitchStripeTexture_.emplace(getContentProperty().Load<Texture2D>("Stripe2"));
        soccerballTexture_.emplace(getContentProperty().Load<Texture2D>("Soccerball"));

        // F1 help overlay (CNA addition beyond the XNA original).
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        eyeAtStart_ = Vector3(0.0f, PlaneSize / 10.0f, -PlaneSize * 0.75f);
        eyeAtBall_ = Vector3(0.0f, 3.0f * SoccerballDiameter, -SoccerballDiameter * 0.75f);

        transparentWhite_ = Color(255, 255, 255, 250); // 250 provides a pleasing additive blend.

        shadowRasterizerState_ = RasterizerState();
        shadowRasterizerState_.setDepthBiasProperty(-SoccerballDepthOffset);

        // Not using getViewportProperty() here: Game::Initialize() calls LoadContent()
        // directly (see Game.cpp), so this runs in the same window where CNA's viewport
        // is known to read stale/wrong values (NEXT.md section 5) -- compute the aspect
        // ratio from the known preferred back-buffer size instead.
        float aspect = (float)PreferredWidth / (float)PreferredHeight;
        projection_ = Matrix::CreatePerspectiveFieldOfView((float)M_PI / 4.0f, aspect, 2.0f, FarClip);
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        if (GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back) ||
            Keyboard::GetState().IsKeyDown(Keys::Escape))
            Exit();

        HandleTouchInput();

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        float time = (float)gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();

        Matrix primitiveOrientation = Matrix::CreateRotationY(time * 0.2f);
        float t = std::max(0.1f, (float)std::sin((double)time * 0.1));
        Vector3 camera = Vector3::Lerp(eyeAtStart_, eyeAtBall_, t);

        Matrix view = Matrix::CreateLookAt(camera, Vector3::Zero, Vector3::Up);

        pitchDualTextureEffect_->setTextureProperty(&*pitchBaseTexture_);
        pitchDualTextureEffect_->setTexture2Property(&*pitchDetailTexture_);
        pitchDualTextureEffect_->setVertexColorEnabledProperty(false);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;
        getGraphicsDeviceProperty().getSamplerStatesProperty()[1] = SamplerState::LinearWrap;

        pitchPrimitive_->DrawDualTextured(*pitchDualTextureEffect_, primitiveOrientation, view, projection_,
                                           Color::White);

        // 2nd pass - add the white pitch-line stripe.
        if (useAlphaBlend_) {
            pitchBasicEffect_->setTextureProperty(&*pitchStripeTexture_);
            pitchBasicEffect_->setTextureEnabledProperty(true);
            pitchBasicEffect_->setLightingEnabledProperty(false);
            pitchStripePrimitive_->Draw(*pitchBasicEffect_, primitiveOrientation, view, projection_,
                                        transparentWhite_);
        } else {
            pitchStripeEffect_->setTextureProperty(&*pitchStripeTexture_);
            pitchStripePrimitive_->DrawAlphaTest(*pitchStripeEffect_, primitiveOrientation, view, projection_,
                                                  Color::White);
        }

        // Render a flattened ball as a shadow.
        Matrix shadowMatrix = Matrix::getIdentityProperty();
        shadowMatrix.M12 = 0.0f;
        shadowMatrix.M22 = 0.0f;
        shadowMatrix.M23 = 0.0f;

        shadowMatrix = primitiveOrientation * shadowMatrix;
        shadowMatrix.M42 = 0.0f;
        pitchBasicEffect_->setTextureEnabledProperty(false);
        pitchBasicEffect_->setLightingEnabledProperty(false);
        RasterizerState oldRasterizerState = getGraphicsDeviceProperty().getRasterizerStateProperty();
        getGraphicsDeviceProperty().setRasterizerStateProperty(shadowRasterizerState_);
        spherePrimitive_->Draw(*pitchBasicEffect_, shadowMatrix, view, projection_, Color::Black);
        getGraphicsDeviceProperty().setRasterizerStateProperty(oldRasterizerState);

        // Render the ball on top of the lot.
        primitiveOrientation.M42 -= -SoccerballRadius;
        pitchBasicEffect_->setTextureProperty(&*soccerballTexture_);
        pitchBasicEffect_->setTextureEnabledProperty(true);
        pitchBasicEffect_->EnableDefaultLighting();
        spherePrimitive_->Draw(*pitchBasicEffect_, primitiveOrientation, view, projection_, Color::White);

        spriteBatch_->Begin();
        spriteBatch_->DrawString(*font_, useAlphaBlend_ ? AlphaBlendText : AlphaTestText, Vector2(320.0f, 70.0f),
                                  Color::White);
        spriteBatch_->End();

        // F1 help overlay (CNA addition beyond the XNA original), drawn in its
        // own screen-space batch -- see NEXT.md section 5 on why a second
        // Begin/End per frame is safe on the default EasyGL backend.
        if (helpTimer_ > 0.0f) {
            spriteBatch_->Begin();
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
            spriteBatch_->End();
        }

        Game::Draw(gameTime);
    }

private:
    static constexpr int PreferredWidth = 480;
    static constexpr int PreferredHeight = 800;

    static constexpr float FarClip = 150.0f;
    static constexpr float PlaneSize = 100.0f;
    static constexpr float PlaneTiling = 30.0f;
    static constexpr float SoccerballDiameter = 2.0f;
    static constexpr float SoccerballRadius = SoccerballDiameter * 0.5f;
    static constexpr float SoccerballDepthOffset = 0.0001f;

    static constexpr const char* AlphaTestText = "Alpha-Test\n";
    static constexpr const char* AlphaBlendText = "Alpha-Blend\n";

    // The original toggles on Touch Released; this desktop has no touchscreen,
    // so a left-click rising edge is treated the same way.
    void HandleTouchInput() {
        for (const auto& location : TouchPanel::GetState()) {
            if (location.getStateProperty() == TouchLocationState::Released) {
                useAlphaBlend_ = !useAlphaBlend_;
            }
        }

        bool mouseDown = Mouse::GetState().getLeftButtonProperty() == ButtonState::Pressed;
        if (!mouseDown && prevMouseDown_) {
            useAlphaBlend_ = !useAlphaBlend_;
        }
        prevMouseDown_ = mouseDown;
    }

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;

    Color transparentWhite_ = Color::White;
    bool useAlphaBlend_ = true;
    bool prevMouseDown_ = false;

    std::optional<PlanePrimitiveDualTextured> pitchPrimitive_;
    std::optional<PlanePrimitiveTextured> pitchStripePrimitive_;
    std::optional<SpherePrimitiveTextured> spherePrimitive_;

    std::optional<Texture2D> pitchBaseTexture_, pitchDetailTexture_, pitchStripeTexture_, soccerballTexture_;

    std::optional<DualTextureEffect> pitchDualTextureEffect_;
    std::optional<BasicEffect> pitchBasicEffect_;
    std::optional<AlphaTestEffect> pitchStripeEffect_;

    RasterizerState shadowRasterizerState_;

    Vector3 eyeAtStart_, eyeAtBall_;
    Matrix projection_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace SoccerPitch
