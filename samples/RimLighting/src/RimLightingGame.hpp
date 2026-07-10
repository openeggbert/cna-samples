#pragma once

// Ported from RimLighting's Game1.cs (SampleGame). Demonstrates how to use
// EnvironmentMapEffect to mimic a rim-lighting effect: the environment-map lookup is
// computed in view space instead of world space (World <- World*View, View <- Identity,
// see RimLighting.htm's "How the Sample Works" section) against a cube map where every
// face is dark except the "back" face (bright) -- producing a rim/silhouette highlight.
//
// NOXNA input substitution: the C# original reads a Windows-Phone-style
// TouchPanel.GetState() touch collection every frame and feeds each active touch point
// to the UI elements (Button/Slidebar) and the ModelViewerCamera's arcballs. CNA's own
// TouchPanel only reports real touch hardware (confirmed via direct source read of
// cna/src/Microsoft/Xna/Framework/Input/Touch/TouchPanel.cpp -- no mouse-to-touch
// synthesis fallback, unlike some other engines), and this dev machine has no
// touchscreen. This port synthesizes a single TouchLocation per frame from
// Mouse::GetState() button-edge transitions instead (Pressed on left-button-down edge,
// Moved while held, Released on left-button-up edge, no TouchLocation at all while
// idle -- matching a real empty TouchCollection) and feeds it through the exact same
// Arcball/Button/Slidebar HandleTouch(TouchLocation) methods the C# original uses,
// unchanged. See missing.md for the full account.

#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Content/ContentManager.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Graphics/EnvironmentMapEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteFont.hpp>
#include <Microsoft/Xna/Framework/Graphics/SurfaceFormat.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/TextureCube.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Buttons.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/MouseState.hpp>
#include <Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp>
#include <Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp>
#include <System/TimeSpan.hpp>

#include "Arcball.hpp"
#include "Button.hpp"
#include "HeadModel.hpp"
#include "ModelViewerCamera.hpp"
#include "Slidebar.hpp"
#include "UIElement.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace RimLightingSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;
using namespace Microsoft::Xna::Framework::Input::Touch;

class RimLightingGame : public Game {
public:
    RimLightingGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        getContentProperty().setRootDirectoryProperty("Content");

        // Frame rate is 30 fps by default for Windows Phone.
        setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333));

        // Pre-autoscale settings.
        graphics_->setPreferredBackBufferWidthProperty(kScreenWidth);
        graphics_->setPreferredBackBufferHeightProperty(kScreenHeight);

        // NOXNA: kept windowed (not graphics.IsFullScreen = true like the original) --
        // desktop dev-loop practicality, matching every other Windows-Phone-style
        // sample in this repo (AccelerometerSample, TiltPerspective, Orientation, ...).
        // See missing.md.
        graphics_->setIsFullScreenProperty(false);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "RimLightingGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        spriteFont_.emplace(getContentProperty().Load<SpriteFont>("Font"));

        // This is the mesh we want to render. NOXNA: bypasses Content.Load<Model> --
        // see HeadModel.hpp's own header comment (DEFERRED.md item #26, plus a real
        // parent-bone transform baked in at conversion time).
        headModel_.Load(getContentProperty(), getGraphicsDeviceProperty(), "head_pasted__polySurface14");

        // An empty white texture used as the default texture for the effect.
        blankTexture_.emplace(getContentProperty().Load<Texture2D>("blankTex"));

        // The cubemap used for generating rim light. NOXNA: bypasses
        // Content.Load<TextureCube> -- see LoadCubemap()'s own comment / missing.md /
        // DEFERRED.md item #14.
        LoadCubemap();

        effect_ = std::make_unique<EnvironmentMapEffect>(getGraphicsDeviceProperty());

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        // Add UI controls.
        GraphicsDevice& device = getGraphicsDeviceProperty();

        buttonToggleWorldCamera_.emplace(device, *spriteFont_, "Rotating World");
        buttonToggleWorldCamera_->SetPosition(Vector2(0.0f, static_cast<float>(kScreenHeight - 31)));
        buttonToggleWorldCamera_->SetSize(Vector2(160.0f, 30.0f));
        buttonToggleWorldCamera_->IsVisible = true;
        buttonToggleWorldCamera_->OnClick = [this]() { OnButtonToggleWorldCameraClick(); };
        uiElements_.push_back(&buttonToggleWorldCamera_.value());

        slideBarEnvironmentMapAmount_.emplace(*spriteFont_, 0.0f, 5.0f);
        slideBarEnvironmentMapAmount_->IsVisible = true;
        slideBarEnvironmentMapAmount_->SetPosition(Vector2(0.0f, static_cast<float>(kScreenHeight - 180)));
        slideBarEnvironmentMapAmount_->OnValueChanged = [this]() { OnEnvironmentMapAmountChanged(); };
        slideBarEnvironmentMapAmount_->SetBlankTexture(*blankTexture_);
        // Note: setting Value must come before SetBarOffsetSize, since the
        // OnValueChanged callback is what makes GetTextSize() valid (used below) --
        // matching the C# original's own ordering comment exactly.
        slideBarEnvironmentMapAmount_->SetValue(2.5f);
        slideBarEnvironmentMapAmount_->SetBarOffsetSize(10.0f, slideBarEnvironmentMapAmount_->GetTextSize().Y,
                                                         static_cast<float>(kScreenWidth - 20), 4.0f);
        uiElements_.push_back(&slideBarEnvironmentMapAmount_.value());

        slideBarFresnelFactor_.emplace(*spriteFont_, 0.0f, 10.0f);
        slideBarFresnelFactor_->IsVisible = true;
        slideBarFresnelFactor_->SetPosition(Vector2(0.0f, static_cast<float>(kScreenHeight - 100)));
        slideBarFresnelFactor_->OnValueChanged = [this]() { OnFresnelFactorChanged(); };
        slideBarFresnelFactor_->SetBlankTexture(*blankTexture_);
        slideBarFresnelFactor_->SetValue(6.0f);
        slideBarFresnelFactor_->SetBarOffsetSize(10.0f, slideBarFresnelFactor_->GetTextSize().Y,
                                                  static_cast<float>(kScreenWidth - 20), 4.0f);
        uiElements_.push_back(&slideBarFresnelFactor_.value());

        Game::LoadContent();
    }

    void Update(GameTime& gameTime) override {
        // F1 help overlay (CNA addition, not in the original -- see CLAUDE.md).
        float elapsed = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // Allows the game to exit. GamePad Back matches the original exactly; Escape is
        // a NOXNA desktop convenience (established repo-wide pattern -- see missing.md).
        if (GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back) ||
            Keyboard::GetState().IsKeyDown(Keys::Escape)) {
            Exit();
        }

        // NOXNA: synthesize a single touch point from mouse input -- see this file's
        // own header comment.
        std::vector<TouchLocation> touches = SynthesizeTouches();

        // Update our UI elements.
        for (const TouchLocation& loc : touches) {
            for (UIElement* element : uiElements_) {
                element->HandleTouch(loc);
            }
        }

        // Update World or View matrices according to the user's drag on screen.
        if (!slideBarEnvironmentMapAmount_->IsDragging && !slideBarFresnelFactor_->IsDragging) {
            modelViewerCamera_.SetIsRotatingWorld(rotatingMode_ == RotatingMode::RotatingWorld);
            for (const TouchLocation& loc : touches) {
                modelViewerCamera_.HandleTouch(loc);
            }
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0.3f, 0.3f, 0.3f));

        // Please refer to the sample doc on why the matrices should be set like this
        // for implementing RimLighting: fake World <- World*View and View <- Identity so
        // the environment-map lookup happens in view space while screen space stays
        // the same.
        Matrix world = modelViewerCamera_.GetWorldMatrix() * modelViewerCamera_.GetViewMatrix();
        Matrix view = Matrix::getIdentityProperty();
        Matrix projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::PiOver4, getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty(), 1.0f,
            10000.0f);

        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        // Draw the model. NOXNA: single hand-loaded mesh instead of iterating
        // model.Meshes/Effects -- see HeadModel.hpp.
        effect_->setEnvironmentMapProperty(&cubemap_.value());

        effect_->setWorldProperty(world);
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);

        effect_->DirectionalLight0.setEnabledProperty(true);
        // Please refer to the sample doc.
        effect_->DirectionalLight0.setDirectionProperty(
            Vector3::TransformNormal(Vector3::Left, modelViewerCamera_.GetViewMatrix()));
        effect_->DirectionalLight1.setEnabledProperty(false);
        effect_->DirectionalLight2.setEnabledProperty(false);

        effect_->setTextureProperty(&blankTexture_.value());

        effect_->setDiffuseColorProperty(Color::White.ToVector3());

        effect_->setFresnelFactorProperty(slideBarFresnelFactor_->GetValue());
        effect_->setEnvironmentMapAmountProperty(slideBarEnvironmentMapAmount_->GetValue());

        for (auto& pass : effect_->getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            headModel_.Draw(getGraphicsDeviceProperty());
        }

        // Draw the button's 3D box outline. Must happen before any SpriteBatch
        // Begin()/End() block -- see UIElement.hpp's own header comment.
        buttonToggleWorldCamera_->DrawBox();

        // Draw our UI elements' text/bars, plus the F1 help overlay, in a single
        // SpriteBatch Begin()/End() block (CLAUDE.md's established convention).
        spriteBatch_->Begin();

        buttonToggleWorldCamera_->DrawText(*spriteBatch_);
        slideBarEnvironmentMapAmount_->DrawText(*spriteBatch_);
        slideBarFresnelFactor_->DrawText(*spriteBatch_);

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = static_cast<float>((vp.getWidthProperty() - hw) / 2);
            float sy = static_cast<float>((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    enum class RotatingMode { RotatingWorld, RotatingCamera };

    static constexpr int kScreenWidth = 480;
    static constexpr int kScreenHeight = 800;

    void OnButtonToggleWorldCameraClick() {
        if (rotatingMode_ == RotatingMode::RotatingWorld) {
            rotatingMode_ = RotatingMode::RotatingCamera;
            buttonToggleWorldCamera_->SetText("Rotating Camera");
        } else {
            rotatingMode_ = RotatingMode::RotatingWorld;
            buttonToggleWorldCamera_->SetText("Rotating World");
        }
    }

    void OnEnvironmentMapAmountChanged() {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Amount: %g", static_cast<double>(slideBarEnvironmentMapAmount_->GetValue()));
        slideBarEnvironmentMapAmount_->SetText(buf);
    }

    void OnFresnelFactorChanged() {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Thickness (FresnelFactor): %g",
                       static_cast<double>(slideBarFresnelFactor_->GetValue()));
        slideBarFresnelFactor_->SetText(buf);
    }

    // NOXNA: synthesizes at most one TouchLocation per frame from Mouse::GetState()'s
    // left-button edge transitions -- see this file's own header comment.
    std::vector<TouchLocation> SynthesizeTouches() {
        constexpr int kSyntheticTouchId = 1; // nonzero -- see Button.hpp's own note.

        MouseState mouse = Mouse::GetState();
        bool leftDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        Vector2 mousePos(static_cast<float>(mouse.getXProperty()), static_cast<float>(mouse.getYProperty()));

        std::vector<TouchLocation> touches;
        if (leftDown && !prevLeftDown_) {
            touches.emplace_back(kSyntheticTouchId, TouchLocationState::Pressed, mousePos);
        } else if (leftDown && prevLeftDown_) {
            touches.emplace_back(kSyntheticTouchId, TouchLocationState::Moved, mousePos);
        } else if (!leftDown && prevLeftDown_) {
            touches.emplace_back(kSyntheticTouchId, TouchLocationState::Released, mousePos);
        }
        prevLeftDown_ = leftDown;
        return touches;
    }

    // Reimplements RimLightingContent's own build-time OutputCube.dds (a real 6-face
    // DDS cubemap, all faces dark except a bright "back" face -- see RimLighting.htm)
    // by loading 6 already-converted PNG faces (via ImageMagick, `convert
    // OutputCube.dds[N] envmap_<face>.png`) through the normal, proven
    // Content.Load<Texture2D> path and copying their pixel data directly into a real
    // CNA TextureCube via its own SetData(CubeMapFace, const Color*, int) API --
    // bypassing ContentManager/TextureCubeTypeReader entirely (DEFERRED.md item #14),
    // the same bypass philosophy as HeadModel.hpp, applied to TextureCube instead of
    // Model. Face order: CNA's own TextureCube::DDSFromStreamEXT() (a NOXNA direct-DDS
    // loader, unusable here since OutputCube.dds is uncompressed xRGB8888, not
    // DXT-compressed) casts DDS face index N directly to CubeMapFace(N) -- i.e. DDS's
    // own on-disk face order is PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ,
    // NegativeZ, matching CNA's CubeMapFace enum's declared order exactly. Confirmed
    // correct live (not just assumed) -- see missing.md.
    void LoadCubemap() {
        constexpr int faceSize = 64;

        struct FaceDef {
            const char* asset;
            CubeMapFace face;
        };
        static constexpr std::array<FaceDef, 6> kFaces = {{
            {"envmap_posx", CubeMapFace::PositiveX},
            {"envmap_negx", CubeMapFace::NegativeX},
            {"envmap_posy", CubeMapFace::PositiveY},
            {"envmap_negy", CubeMapFace::NegativeY},
            {"envmap_posz", CubeMapFace::PositiveZ},
            {"envmap_negz", CubeMapFace::NegativeZ},
        }};

        cubemap_.emplace(getGraphicsDeviceProperty(), faceSize, false, SurfaceFormat::Color);

        std::vector<Color> pixels(static_cast<std::size_t>(faceSize) * static_cast<std::size_t>(faceSize),
                                   Color(0, 0, 0, 0));

        for (const auto& faceDef : kFaces) {
            Texture2D faceTexture = getContentProperty().Load<Texture2D>(faceDef.asset);
            faceTexture.GetData(pixels.data(), static_cast<int>(pixels.size()));
            cubemap_->SetData(faceDef.face, pixels.data(), static_cast<int>(pixels.size()));
        }
    }

    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> spriteFont_;

    // Whether we are rotating world or camera.
    RotatingMode rotatingMode_ = RotatingMode::RotatingWorld;

    // Button to switch between the two rotating modes above.
    std::optional<Button> buttonToggleWorldCamera_;

    // Slidebars to tweak the effect.
    std::optional<Slidebar> slideBarEnvironmentMapAmount_;
    std::optional<Slidebar> slideBarFresnelFactor_;

    // List of all UI elements, for touch dispatch (drawing is done per-concrete-type --
    // see Draw()).
    std::vector<UIElement*> uiElements_;

    // NOXNA: CameraInitPosition = new Vector3(0, 0, -60) in the original.
    ModelViewerCamera modelViewerCamera_{Vector3(0.0f, 0.0f, -60.0f), Vector3::Up, 0, 0, kScreenWidth, kScreenHeight};

    // The mesh to be rendered. NOXNA: HeadModel bypasses Content.Load<Model> -- see
    // HeadModel.hpp.
    HeadModel headModel_;

    // The cube texture for the rim-lighting effect.
    std::optional<TextureCube> cubemap_;

    // Default texture for the mesh.
    std::optional<Texture2D> blankTexture_;

    std::unique_ptr<EnvironmentMapEffect> effect_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    // NOXNA: tracks the previous frame's mouse left-button state, for touch synthesis.
    bool prevLeftDown_ = false;
};

} // namespace RimLightingSample
