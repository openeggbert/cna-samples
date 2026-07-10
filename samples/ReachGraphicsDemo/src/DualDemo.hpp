#pragma once

// Ported from XnaGraphicsDemo.DualDemo (DualDemo.cs). Demo shows how to use
// DualTextureEffect: a small 7-part scene (a ground plane + 6 blocks) with a base
// texture plus a light-map overlay texture, with menu toggles to disable either layer
// (swapped to a flat grey texture, matching the C# original's own toggle mechanic) and
// drag-to-orbit camera controls.
//
// model.fbx's own 7 meshes (pPlane1, pCube15..pCube20) are all flat, direct children of
// the FBX scene root (confirmed via its own Connect: "OO" lines -- unlike tank.fbx, no
// nested bone hierarchy here), so the standard (baking) tools/fbx_ascii2model.py
// conversion already applies each mesh's own local node transform directly to its
// vertex data -- no per-mesh runtime transform is needed, every submesh can be drawn
// with the same shared `rotation` world matrix, exactly mirroring what XNA's
// `transforms[mesh.ParentBone.Index]` would evaluate to (identity) for this flat
// single-bone-per-mesh case. See RawMesh.hpp for why Content.Load<Model>("model") isn't
// used directly (DEFERRED.md item #26). Uses RawMeshPosTex.hpp (not RawMesh.hpp) for
// its own vertex upload -- see that header's own comment for why DualTextureEffect
// specifically needs a Position+UV-only vertex layout, unlike every other effect type
// used in this sample.

#include <array>
#include <optional>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DualTextureEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "DemoGame.hpp"
#include "MenuComponent.hpp"
#include "MenuEntry.hpp"
#include "RawMeshPosTex.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class DualDemo : public MenuComponent {
public:
    explicit DualDemo(DemoGame& game) : MenuComponent(game) {
        auto showTextureEntry = std::make_unique<BoolMenuEntry>("texture");
        showTexture_ = showTextureEntry.get();
        Entries.push_back(std::move(showTextureEntry));

        auto showLightmapEntry = std::make_unique<BoolMenuEntry>("light map");
        showLightmap_ = showLightmapEntry.get();
        Entries.push_back(std::move(showLightmapEntry));

        auto backEntry = std::make_unique<MenuEntry>();
        backEntry->SetText("back");
        backEntry->Clicked = [&game]() { game.SetActiveMenu(0); };
        Entries.push_back(std::move(backEntry));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "DualDemo";
        return name;
    }

    // Resets the menu state.
    void Reset() override {
        showTexture_->Value = true;
        showLightmap_->Value = true;

        cameraRotation_ = 124.0f;
        cameraArc_ = -12.0f;

        MenuComponent::Reset();
    }

    // Loads content for this demo.
    void LoadContent() override {
        Content::ContentManager& content = GetGame().getContentProperty();
        const std::string root = content.getRootDirectoryProperty();

        for (std::size_t i = 0; i < kMeshCount; ++i) {
            meshes_[i].Load(root, std::string("model_") + kMeshes[i].name, getGraphicsDeviceProperty());
        }

        grassTexture_ = content.Load<Texture2D>("grass1");
        tileTexture_ = content.Load<Texture2D>("tile1");
        lightmapTexture_ = content.Load<Texture2D>("lightmap");

        grey_.emplace(getGraphicsDeviceProperty(), 1, 1);
        Color grey(128, 128, 128, 255);
        grey_->SetData(&grey, 1);

        effect_ = std::make_unique<DualTextureEffect>(getGraphicsDeviceProperty());
    }

    // Draws the DualTextureEffect demo.
    void Draw(const GameTime& gameTime) override {
        DrawTitle("dual texture effect", Color(128, 160, 128, 255), Color(96, 128, 96, 255));

        // Compute camera matrices.
        Matrix rotation =
            Matrix::CreateRotationY(MathHelper::ToRadians(cameraRotation_)) * Matrix::CreateRotationZ(MathHelper::ToRadians(cameraArc_));

        Matrix view = Matrix::CreateLookAt(Vector3(35.0f, 13.0f, 0.0f), Vector3(0.0f, 3.0f, 0.0f), Vector3::Up);

        Matrix projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::PiOver4, getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty(), 2.0f, 100.0f);

        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        effect_->setViewProperty(view);
        effect_->setProjectionProperty(projection);
        effect_->setDiffuseColorProperty(Vector3(0.75f, 0.75f, 0.75f));

        for (std::size_t i = 0; i < kMeshCount; ++i) {
            effect_->setWorldProperty(rotation);

            effect_->setTextureProperty(showTexture_->Value ? kMeshes[i].usesGrass ? &grassTexture_.value() : &tileTexture_.value()
                                                              : &grey_.value());
            effect_->setTexture2Property(showLightmap_->Value ? &lightmapTexture_.value() : &grey_.value());

            meshes_[i].Draw(*effect_, getGraphicsDeviceProperty());
        }

        MenuComponent::Draw(gameTime);
    }

protected:
    // Dragging on the menu background rotates the camera.
    void OnDrag(Vector2 delta) override {
        cameraRotation_ = MathHelper::Clamp(cameraRotation_ + delta.X / 8.0f, 0.0f, 180.0f);
        cameraArc_ = MathHelper::Clamp(cameraArc_ - delta.Y / 8.0f, -50.0f, 15.0f);
    }

private:
    struct DualMeshDef {
        const char* name;
        bool usesGrass; // true: grass1.png, false: tile1.png
    };

    static constexpr std::size_t kMeshCount = 7;
    inline static const std::array<DualMeshDef, kMeshCount> kMeshes = {{
        {"pPlane1", true},
        {"pCube15", false},
        {"pCube16", false},
        {"pCube17", false},
        {"pCube18", false},
        {"pCube19", false},
        {"pCube20", false},
    }};

    std::array<RawMeshPosTex, kMeshCount> meshes_;
    std::optional<Texture2D> grassTexture_;
    std::optional<Texture2D> tileTexture_;
    std::optional<Texture2D> lightmapTexture_;
    std::optional<Texture2D> grey_;
    std::unique_ptr<DualTextureEffect> effect_;

    BoolMenuEntry* showTexture_ = nullptr;
    BoolMenuEntry* showLightmap_ = nullptr;

    float cameraRotation_ = 0.0f;
    float cameraArc_ = 0.0f;
};

} // namespace ReachGraphicsDemoSample
