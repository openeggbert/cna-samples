#pragma once

// NOXNA helper -- ported from XnaGraphicsDemo/SimpleAnimation's Tank.cs (the "helper
// class for drawing a tank model with animated wheels and turret" shared by
// BasicDemo.cs and AlphaDemo.cs), reimplemented on top of RawMesh.hpp instead of
// Content.Load<Model>("tank") for two independent reasons:
//
// 1. DEFERRED.md item #26 (ModelTypeReader vertex-stride/IVertexType-vtable mismatch)
//    -- see RawMesh.hpp's own header comment for the full mechanism. Every
//    stride-32 .model.json in this repo is affected.
// 2. tank.fbx's own Connect: "OO" lines reveal a REAL, nested (parent, child) bone
//    hierarchy (tank_geo -> {r_engine_geo, l_engine_geo, turret_geo}; r_engine_geo ->
//    {r_back_wheel_geo, r_steer_geo}; r_steer_geo -> r_front_wheel_geo; turret_geo ->
//    {canon_geo, hatch_geo}; mirrored for the left side) -- unlike every other model in
//    this repo (all flat, single-bone). CNA's `.model.json`/ModelTypeReader has no
//    per-mesh ModelBone/parent-bone support at all (DEFERRED.md item #6's multi-bone
//    addendum -- the same gap blocking SplitScreen/TankOnAHeightMap/SimpleAnimation),
//    so `Content.Load<Model>("tank")` could not reproduce Tank.cs's independently
//    rotating wheels/steering/turret/cannon/hatch even if item #26 didn't exist.
//
// Since this bypasses CNA's Model/ModelBone system entirely (same as RawMesh.hpp),
// there is no CNA-side ModelBone hierarchy to be missing in the first place -- the
// hierarchy below is simply reimplemented directly in C++, replicating exactly the same
// `absolute[mesh] = local[mesh] * absolute[parent]` chain XNA's own
// Model.CopyAbsoluteBoneTransformsTo() performs, with `local[mesh]` recomputed every
// frame from Tank.cs's own `<rotation> * <original bone Transform>` formula. A one-off
// conversion script (not committed to tools/, see missing.md) extracted each of
// tank.fbx's 12 named Model nodes' own mesh-local (UN-baked -- see RawMesh.hpp) vertex
// data plus each node's own local rest translation (confirmed, via
// tools/fbx_ascii2model.py's own parse_model_transform(), that every one of these 12
// nodes has identity rotation/scale -- only translation -- so a Vector3 offset is all
// that is needed here, no 3x3 rotation matrix).
//
// tank_geo (the root bone) is intentionally NOT given a rest transform here: Tank.cs's
// own Draw() unconditionally overwrites it every frame (`tankModel.Root.Transform =
// world;`), discarding whatever rest transform the content pipeline would have baked in
// -- reproduced exactly by simply using the passed-in `world` matrix directly for
// tank_geo's own absolute transform.

#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "RawMesh.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

// Mirrors BasicDemo.cs's LightingMode enum (also used by AlphaDemo.cs).
enum class LightingMode {
    NoLighting,
    OneVertexLight,
    ThreeVertexLights,
    ThreePixelLights,
};

class TankModel {
public:
    // Loads the tank model.
    void Load(Content::ContentManager& content, GraphicsDevice& device) {
        const std::string root = content.getRootDirectoryProperty();

        for (std::size_t i = 0; i < kPartCount; ++i) {
            parts_[i].Load(root, std::string("tank_") + kParts[i].name, device);
        }

        engineTexture_ = content.Load<Texture2D>("engine_diff_tex");
        turretTexture_ = content.Load<Texture2D>("turret_alt_diff_tex");

        effect_ = std::make_unique<BasicEffect>(device);
        effect_->setTextureEnabledProperty(true);
    }

    // Animates the tank model.
    void Animate(const GameTime& gameTime) {
        float time = static_cast<float>(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());

        steerRotationValue_ = std::sin(time * 0.75f) * 0.5f;
        turretRotationValue_ = std::sin(time * 0.333f) * 1.25f;
        cannonRotationValue_ = std::sin(time * 0.25f) * 0.333f - 0.333f;
        hatchRotationValue_ = MathHelper::Clamp(std::sin(time * 2.0f) * 2.0f, -1.0f, 0.0f);
        // NOXNA note: WheelRotation is never set by Tank.cs's own Animate() either --
        // confirmed by a full-file grep of Tank.cs/BasicDemo.cs/AlphaDemo.cs -- so the
        // wheels never actually spin in the real XNA original. Kept as a dead property
        // (always 0) for exact structural fidelity with Tank.cs's own Draw(), which
        // still unconditionally builds and applies a `wheelRotation` matrix from it.
    }

    // Draws the tank model, using the current animation settings.
    void Draw(const Matrix& world, const Matrix& view, const Matrix& projection, LightingMode lightMode,
              bool textureEnable) {
        Matrix wheelRotation = Matrix::CreateRotationX(wheelRotationValue_);
        Matrix steerRotation = Matrix::CreateRotationY(steerRotationValue_);
        Matrix turretRotation = Matrix::CreateRotationY(turretRotationValue_);
        Matrix cannonRotation = Matrix::CreateRotationX(cannonRotationValue_);
        Matrix hatchRotation = Matrix::CreateRotationX(hatchRotationValue_);

        // Look up combined bone matrices for the entire model (mirrors
        // Model.CopyAbsoluteBoneTransformsTo()).
        std::array<Matrix, kPartCount> absolute;

        for (std::size_t i = 0; i < kPartCount; ++i) {
            const TankPartDef& def = kParts[i];

            Matrix local = Matrix::CreateTranslation(def.restTranslation);
            switch (def.animGroup) {
                case TankAnimGroup::Wheel: local = wheelRotation * local; break;
                case TankAnimGroup::Steer: local = steerRotation * local; break;
                case TankAnimGroup::Turret: local = turretRotation * local; break;
                case TankAnimGroup::Cannon: local = cannonRotation * local; break;
                case TankAnimGroup::Hatch: local = hatchRotation * local; break;
                case TankAnimGroup::None: break;
            }

            const Matrix& parentAbsolute = (def.parentIndex < 0) ? world : absolute[static_cast<std::size_t>(def.parentIndex)];
            absolute[i] = local * parentAbsolute;
        }

        effect_->View = view;
        effect_->Projection = projection;

        switch (lightMode) {
            case LightingMode::NoLighting:
                effect_->setLightingEnabledProperty(false);
                break;

            case LightingMode::OneVertexLight:
                effect_->EnableDefaultLighting();
                effect_->setPreferPerPixelLightingProperty(false);
                effect_->getDirectionalLight1Property().setEnabledProperty(false);
                effect_->getDirectionalLight2Property().setEnabledProperty(false);
                break;

            case LightingMode::ThreeVertexLights:
                effect_->EnableDefaultLighting();
                effect_->setPreferPerPixelLightingProperty(false);
                break;

            case LightingMode::ThreePixelLights:
                effect_->EnableDefaultLighting();
                effect_->setPreferPerPixelLightingProperty(true);
                break;
        }

        effect_->setSpecularColorProperty(Vector3(0.8f, 0.8f, 0.6f));
        effect_->setSpecularPowerProperty(16.0f);
        effect_->setTextureEnabledProperty(textureEnable);

        auto& device = effect_->getGraphicsDeviceInternal();

        for (std::size_t i = 0; i < kPartCount; ++i) {
            effect_->World = absolute[i];
            effect_->setTextureProperty(kParts[i].usesTurretTexture ? &turretTexture_.value() : &engineTexture_.value());
            parts_[i].Draw(*effect_, device);
        }
    }

private:
    enum class TankAnimGroup { None, Wheel, Steer, Turret, Cannon, Hatch };

    struct TankPartDef {
        const char* name;
        int parentIndex; // index into kParts, -1 for the root (tank_geo)
        TankAnimGroup animGroup;
        Vector3 restTranslation;
        bool usesTurretTexture; // true: turret_alt_diff_tex.tga, false: engine_diff_tex.tga
    };

    static constexpr std::size_t kPartCount = 12;

    // Order matters: every part's parentIndex must refer to an EARLIER entry.
    // NOXNA note: Vector3 has no constexpr constructor, so this table (unlike kPartCount
    // above) is a plain (non-constexpr) inline static const, initialized once at load
    // time.
    inline static const std::array<TankPartDef, kPartCount> kParts = {{
        {"tank_geo", -1, TankAnimGroup::None, Vector3(0.0f, 0.0f, 0.0f), true},
        {"r_engine_geo", 0, TankAnimGroup::None, Vector3(-139.500503540039f, 163.812591552734f, -10.956600189209f), false},
        {"l_engine_geo", 0, TankAnimGroup::None, Vector3(139.500503540039f, 163.812591552734f, -10.956600189209f), false},
        {"turret_geo", 0, TankAnimGroup::Turret, Vector3(0.0f, 231.753982543945f, -35.5949974060059f), true},
        {"r_back_wheel_geo", 1, TankAnimGroup::Wheel, Vector3(-134.905899047852f, -58.926513671875f, -234.273040771484f), false},
        {"r_steer_geo", 1, TankAnimGroup::Steer, Vector3(-31.7506103515625f, 22.1607513427734f, 251.485000610352f), false},
        {"r_front_wheel_geo", 5, TankAnimGroup::Wheel, Vector3(85.2137145996094f, -112.978843688965f, -1.13240051269531f), false},
        {"l_back_wheel_geo", 2, TankAnimGroup::Wheel, Vector3(134.905899047852f, -58.926513671875f, -234.273040771484f), false},
        {"l_steer_geo", 2, TankAnimGroup::Steer, Vector3(31.7506103515625f, 22.1607513427734f, 251.485000610352f), false},
        {"l_front_wheel_geo", 8, TankAnimGroup::Wheel, Vector3(-85.2137145996094f, -112.978843688965f, -1.13240051269531f), false},
        {"canon_geo", 3, TankAnimGroup::Cannon, Vector3(0.0f, 104.642013549805f, 102.743896484375f), true},
        {"hatch_geo", 3, TankAnimGroup::Hatch, Vector3(62.9840126037598f, 125.905166625977f, -43.5861015319824f), true},
    }};

    std::array<RawMesh, kPartCount> parts_;
    std::optional<Texture2D> engineTexture_;
    std::optional<Texture2D> turretTexture_;
    std::unique_ptr<BasicEffect> effect_;

    // Current animation positions.
    float wheelRotationValue_ = 0.0f;
    float steerRotationValue_ = 0.0f;
    float turretRotationValue_ = 0.0f;
    float cannonRotationValue_ = 0.0f;
    float hatchRotationValue_ = 0.0f;
};

} // namespace ReachGraphicsDemoSample
