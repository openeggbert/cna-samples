#pragma once

// Ported from XnaGraphicsDemo.EnvmapDemo (EnvmapDemo.cs). Demo shows how to use
// EnvironmentMapEffect: a spinning saucer model reflecting a 6-face cubemap over a full
// screen 2D background image, with 3 slider menu entries controlling the environment
// map blend amount / Fresnel factor / specular tint.
//
// The cubemap itself is CNA's own item (DEFERRED.md item #14: no
// Content.Load<TextureCube>/TextureCubeTypeReader) -- this port does NOT add one (out of
// scope for a porting session, per this task's own brief). Instead, the C# original's
// own ContentPipelineExtension/CubemapProcessor.cs build-time algorithm (mirror the
// source image left/right so it wraps seamlessly, crop 4 "side" faces, fold/warp the
// top and bottom thirds into the remaining 2 faces, blur the seam) was reimplemented as
// a one-off Python/Pillow script (not committed to tools/ -- see missing.md), run once
// against seattle.bmp + seattle_alpha.bmp (merged the same way
// TexturePlusAlphaProcessor.cs does: alpha = average of the alpha bitmap's own R/G/B) to
// produce 6 ordinary PNG face images (envmap_{posx,negx,posy,negy,posz,negz}.png).
// Those are loaded here via the normal Content.Load<Texture2D> path (proven working)
// and their pixel data copied directly into a real CNA TextureCube via its own
// SetData(CubeMapFace, const Color*, int) API -- bypassing ContentManager/
// TextureCubeTypeReader entirely, the same "construct the real C++ object directly"
// bypass philosophy as RawMesh.hpp, just applied to a different CNA type.
//
// Two real CNA-level rendering gaps found and worked around while porting this scene
// (both confirmed live via screenshot isolation -- see missing.md for the full account):
//
// 1. DEFERRED.md item #24 (GraphicsDevice::Clear(Color)'s single-arg overload never
//    clears the depth buffer): worked around here with the 2-arg Clear(Color, float)
//    overload (which does clear depth), since this scene's own saucer draw depends on
//    depth testing against a fresh buffer every frame, unlike every other demo in this
//    sample (whose own 3D content is either the very first thing drawn each frame, or
//    doesn't rely on cross-frame depth-buffer isolation).
// 2. A new, narrower CNA gap: a `SpriteBatch.Begin()/Draw()/End()` call that draws a
//    texture stretched to cover the *entire* backbuffer, executed before any 3D
//    `DrawIndexedPrimitives` call in the same frame, leaves backend state that makes
//    every subsequent 3D draw call in that frame render nothing -- confirmed live,
//    isolated step by step: (a) removing just this one SpriteBatch.Draw() call made the
//    saucer render correctly; (b) swapping the 3D effect (EnvironmentMapEffect ->
//    plain BasicEffect), changing the camera to a much closer/simpler one, and forcing
//    RasterizerState::CullNone each independently made no difference (still invisible)
//    as long as the full-screen SpriteBatch draw remained; (c) converting the source
//    image from JPEG to an ordinary RGBA PNG also made no difference -- ruling out both
//    the effect type/camera and the source texture's own format as the cause, and
//    narrowing it specifically to "a SpriteBatch draw covering the whole backbuffer,
//    positioned before 3D content in the same frame." Not fully root-caused inside cna
//    itself (out of scope to fix here per this task's own "do not edit cna" constraint,
//    and this sample's own explicit BlendState/RasterizerState/DepthStencilState/
//    SamplerState resets before the 3D draw call do NOT clear whatever state is left
//    dirty). Worked around at the sample level: the background image is drawn as a
//    manually-built full-screen quad via BasicEffect + DrawUserIndexedPrimitives
//    (DrawBackgroundQuad(), NOXNA) instead of SpriteBatch, sidestepping the SpriteBatch
//    call entirely for this one draw (DrawTitle()'s own, much smaller SpriteBatch call
//    immediately after it is unaffected, confirmed live once the background switched to
//    this approach). Filed as a new DEFERRED.md item.

#include <array>
#include <optional>
#include <vector>

#include <cstdint>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/CubeMapFace.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/EnvironmentMapEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/TextureCube.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionTexture.hpp"

#include "DemoGame.hpp"
#include "MenuComponent.hpp"
#include "MenuEntry.hpp"
#include "RawMesh.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class EnvmapDemo : public MenuComponent {
public:
    explicit EnvmapDemo(DemoGame& game) : MenuComponent(game) {
        auto amountEntry = std::make_unique<FloatMenuEntry>();
        amountEntry->SetText("envmap");
        amount_ = amountEntry.get();
        Entries.push_back(std::move(amountEntry));

        auto fresnelEntry = std::make_unique<FloatMenuEntry>();
        fresnelEntry->SetText("fresnel");
        fresnel_ = fresnelEntry.get();
        Entries.push_back(std::move(fresnelEntry));

        auto specularEntry = std::make_unique<FloatMenuEntry>();
        specularEntry->SetText("specular");
        specular_ = specularEntry.get();
        Entries.push_back(std::move(specularEntry));

        auto backEntry = std::make_unique<MenuEntry>();
        backEntry->SetText("back");
        backEntry->Clicked = [&game]() { game.SetActiveMenu(0); };
        Entries.push_back(std::move(backEntry));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "EnvmapDemo";
        return name;
    }

    // Resets the menu state.
    void Reset() override {
        amount_->Value = 1.0f;
        fresnel_->Value = 0.25f;
        specular_->Value = 0.5f;

        MenuComponent::Reset();
    }

    // Loads content for this demo.
    void LoadContent() override {
        Content::ContentManager& content = GetGame().getContentProperty();

        background_ = content.Load<Texture2D>("background");

        saucer_.Load(content.getRootDirectoryProperty(), "saucer_p1_saucer_geo", getGraphicsDeviceProperty());
        saucerTexture_ = content.Load<Texture2D>("saucer_texture");

        LoadCubemap(content);

        effect_ = std::make_unique<EnvironmentMapEffect>(getGraphicsDeviceProperty());
        effect_->setTextureProperty(&saucerTexture_.value());
        effect_->setEnvironmentMapProperty(&cubemap_.value());

        backgroundEffect_ = std::make_unique<BasicEffect>(getGraphicsDeviceProperty());
        backgroundEffect_->setTextureEnabledProperty(true);
        backgroundEffect_->setTextureProperty(&background_.value());
        backgroundEffect_->World = Matrix::getIdentityProperty();
        backgroundEffect_->View = Matrix::getIdentityProperty();
        backgroundEffect_->Projection = Matrix::getIdentityProperty();
    }

    // Draws the EnvironmentMapEffect demo.
    void Draw(const GameTime& gameTime) override {
        // NOXNA: single-arg Clear(Color) never clears the depth buffer in CNA
        // (DEFERRED.md item #24) -- use the 2-arg overload (also clears depth) so
        // stale depth from a previous frame/scene can't block the saucer's depth test.
        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 255), 1.0f);

        // Draw the background image (see this file's own header comment for why this
        // is a manually-built full-screen quad, not a SpriteBatch draw).
        DrawBackgroundQuad();

        DrawTitle("environment map effect", std::optional<Color>(), Color(93, 142, 196, 255));

        // Compute camera matrices.
        float time = static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());

        Matrix rotation = Matrix::CreateRotationX(time * 0.3f) * Matrix::CreateRotationY(time);

        Matrix view = Matrix::CreateLookAt(Vector3(4500.0f, -400.0f, 0.0f), Vector3(0.0f, -400.0f, 0.0f), Vector3::Up);

        Matrix projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::PiOver4, getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty(), 10.0f,
            10000.0f);

        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        // Draw the spaceship model.
        effect_->setWorldProperty(rotation);
        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);

        effect_->EnableDefaultLighting();

        effect_->setEnvironmentMapAmountProperty(amount_->Value);
        effect_->setFresnelFactorProperty(fresnel_->Value * 2.0f);
        effect_->setEnvironmentMapSpecularProperty(Vector3(1.0f, 1.0f, 0.5f) * specular_->Value);

        saucer_.Draw(*effect_, getGraphicsDeviceProperty());

        MenuComponent::Draw(gameTime);
    }

private:
    // NOXNA: draws the full-screen background image as a manually-built quad via
    // BasicEffect + DrawUserIndexedPrimitives, instead of SpriteBatch -- see this
    // file's own header comment for why. World/View/Projection are all Identity, so
    // the quad's own vertex positions (below) are effectively already in clip space.
    void DrawBackgroundQuad() {
        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullNone);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::None);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearClamp;

        const VertexPositionTexture verts[4] = {
            VertexPositionTexture(Vector3(-1.0f, 1.0f, 0.0f), Vector2(0.0f, 0.0f)),
            VertexPositionTexture(Vector3(1.0f, 1.0f, 0.0f), Vector2(1.0f, 0.0f)),
            VertexPositionTexture(Vector3(1.0f, -1.0f, 0.0f), Vector2(1.0f, 1.0f)),
            VertexPositionTexture(Vector3(-1.0f, -1.0f, 0.0f), Vector2(0.0f, 1.0f)),
        };
        const std::uint16_t indices[6] = {0, 1, 2, 0, 2, 3};

        for (auto& pass : backgroundEffect_->getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            getGraphicsDeviceProperty().DrawUserIndexedPrimitives(PrimitiveType::TriangleList, verts, 0, 4, indices, 0, 2);
        }
    }

    // Reimplements CubemapProcessor.cs's own algorithm's *output* (as a one-off Python
    // script, see missing.md) into 6 already-converted PNG faces; this method just
    // loads each one via the normal, proven Content.Load<Texture2D> path and copies its
    // pixel data into a real TextureCube, bypassing ContentManager/
    // TextureCubeTypeReader (DEFERRED.md item #14) entirely.
    void LoadCubemap(Content::ContentManager& content) {
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
            Texture2D faceTexture = content.Load<Texture2D>(faceDef.asset);
            faceTexture.GetData(pixels.data(), static_cast<int>(pixels.size()));
            cubemap_->SetData(faceDef.face, pixels.data(), static_cast<int>(pixels.size()));
        }
    }

    RawMesh saucer_;
    std::optional<Texture2D> saucerTexture_;
    std::optional<Texture2D> background_;
    std::optional<TextureCube> cubemap_;
    std::unique_ptr<EnvironmentMapEffect> effect_;
    std::unique_ptr<BasicEffect> backgroundEffect_;

    FloatMenuEntry* amount_ = nullptr;
    FloatMenuEntry* fresnel_ = nullptr;
    FloatMenuEntry* specular_ = nullptr;
};

} // namespace ReachGraphicsDemoSample
