#pragma once

// Direct port of SimpleAnimation.cs's Tank.cs -- helper class for drawing a
// tank model with animated wheels, steering, turret, cannon, and hatch.

#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/Model.hpp"
#include "Microsoft/Xna/Framework/Graphics/ModelBone.hpp"
#include "Microsoft/Xna/Framework/Graphics/ModelMesh.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"

#include <optional>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Content;

namespace SimpleAnimation {

class Tank {
public:
    // Gets or sets the wheel rotation amount.
    float WheelRotation  = 0.0f;
    // Gets or sets the steering rotation amount.
    float SteerRotation  = 0.0f;
    // Gets or sets the turret rotation amount.
    float TurretRotation = 0.0f;
    // Gets or sets the cannon rotation amount.
    float CannonRotation = 0.0f;
    // Gets or sets the entry hatch rotation amount.
    float HatchRotation  = 0.0f;

    // Loads the tank model.
    void Load(ContentManager& content) {
        // Load the tank model from the ContentManager.
        tankModel_ = content.Load<Model>("tank");

        // NOXNA: cna's ModelTypeReader (Task 937, cna commit landed 2026-07-10)
        // gives every mesh in a .model.json its own real ModelBone -- unblocking
        // this sample -- but always parents it directly to the model's synthetic
        // Root bone with an Identity Transform. There is no per-mesh rest-transform
        // field in the .model.json schema (yet) and no nested-bone-hierarchy support,
        // unlike real XNA's own content pipeline. tank.fbx's own node hierarchy is
        // genuinely nested (e.g. l_back_wheel_geo is a child of l_engine_geo, which
        // is a child of tank_geo) and every node's own Lcl Translation is non-zero --
        // so the bone Transform real Tank.cs captures via `leftBackWheelBone.Transform`
        // would, in real XNA, already hold the correct rest offset. Here it would be
        // Identity without this fix, and every part would render stacked at the tank
        // body's own local origin. See missing.md for the full derivation (values read
        // directly from tank.fbx's own Lcl Translation properties, composed through the
        // real parent chain -- every rotation/PreRotation in this asset is zero, so
        // composing is a plain vector sum, confirmed by direct source read).
        ApplyRestTransforms();

        // Look up shortcut references to the bones we are going to animate.
        leftBackWheelBone_   = tankModel_->getBonesProperty()["l_back_wheel_geo"];
        rightBackWheelBone_  = tankModel_->getBonesProperty()["r_back_wheel_geo"];
        leftFrontWheelBone_  = tankModel_->getBonesProperty()["l_front_wheel_geo"];
        rightFrontWheelBone_ = tankModel_->getBonesProperty()["r_front_wheel_geo"];
        leftSteerBone_       = tankModel_->getBonesProperty()["l_steer_geo"];
        rightSteerBone_      = tankModel_->getBonesProperty()["r_steer_geo"];
        turretBone_          = tankModel_->getBonesProperty()["turret_geo"];
        cannonBone_          = tankModel_->getBonesProperty()["canon_geo"];
        hatchBone_           = tankModel_->getBonesProperty()["hatch_geo"];

        // Store the original transform matrix for each animating bone.
        leftBackWheelTransform_   = leftBackWheelBone_->getTransformProperty();
        rightBackWheelTransform_  = rightBackWheelBone_->getTransformProperty();
        leftFrontWheelTransform_  = leftFrontWheelBone_->getTransformProperty();
        rightFrontWheelTransform_ = rightFrontWheelBone_->getTransformProperty();
        leftSteerTransform_       = leftSteerBone_->getTransformProperty();
        rightSteerTransform_      = rightSteerBone_->getTransformProperty();
        turretTransform_          = turretBone_->getTransformProperty();
        cannonTransform_          = cannonBone_->getTransformProperty();
        hatchTransform_           = hatchBone_->getTransformProperty();

        // Allocate the transform matrix array.
        boneTransforms_.resize(static_cast<std::size_t>(tankModel_->getBonesProperty().getCountProperty()));
    }

    // Draws the tank model, using the current animation settings.
    void Draw(const Matrix& world, const Matrix& view, const Matrix& projection) {
        // Set the world matrix as the root transform of the model.
        tankModel_->getRootProperty()->setTransformProperty(world);

        // Calculate matrices based on the current animation position.
        Matrix wheelRotation  = Matrix::CreateRotationX(WheelRotation);
        Matrix steerRotation  = Matrix::CreateRotationY(SteerRotation);
        Matrix turretRotation = Matrix::CreateRotationY(TurretRotation);
        Matrix cannonRotation = Matrix::CreateRotationX(CannonRotation);
        Matrix hatchRotation  = Matrix::CreateRotationX(HatchRotation);

        // Apply matrices to the relevant bones.
        leftBackWheelBone_->setTransformProperty(wheelRotation * leftBackWheelTransform_);
        rightBackWheelBone_->setTransformProperty(wheelRotation * rightBackWheelTransform_);
        leftFrontWheelBone_->setTransformProperty(wheelRotation * leftFrontWheelTransform_);
        rightFrontWheelBone_->setTransformProperty(wheelRotation * rightFrontWheelTransform_);
        leftSteerBone_->setTransformProperty(steerRotation * leftSteerTransform_);
        rightSteerBone_->setTransformProperty(steerRotation * rightSteerTransform_);
        turretBone_->setTransformProperty(turretRotation * turretTransform_);
        cannonBone_->setTransformProperty(cannonRotation * cannonTransform_);
        hatchBone_->setTransformProperty(hatchRotation * hatchTransform_);

        // Look up combined bone matrices for the entire model.
        tankModel_->CopyAbsoluteBoneTransformsTo(boneTransforms_);

        // Draw the model.
        for (ModelMesh* mesh : tankModel_->getMeshesProperty()) {
            for (Effect* effect : mesh->getEffectsPropertyMutable()) {
                auto* basicEffect = static_cast<BasicEffect*>(effect);

                basicEffect->World      = boneTransforms_[static_cast<std::size_t>(mesh->getParentBoneProperty()->getIndexProperty())];
                basicEffect->View       = view;
                basicEffect->Projection = projection;

                basicEffect->EnableDefaultLighting();
            }

            mesh->Draw();
        }
    }

private:
    // The XNA framework Model object that we are going to display.
    std::optional<Model> tankModel_;

    // Shortcut references to the bones that we are going to animate.
    ModelBone* leftBackWheelBone_   = nullptr;
    ModelBone* rightBackWheelBone_  = nullptr;
    ModelBone* leftFrontWheelBone_  = nullptr;
    ModelBone* rightFrontWheelBone_ = nullptr;
    ModelBone* leftSteerBone_       = nullptr;
    ModelBone* rightSteerBone_      = nullptr;
    ModelBone* turretBone_          = nullptr;
    ModelBone* cannonBone_          = nullptr;
    ModelBone* hatchBone_           = nullptr;

    // Store the original transform matrix for each animating bone.
    Matrix leftBackWheelTransform_;
    Matrix rightBackWheelTransform_;
    Matrix leftFrontWheelTransform_;
    Matrix rightFrontWheelTransform_;
    Matrix leftSteerTransform_;
    Matrix rightSteerTransform_;
    Matrix turretTransform_;
    Matrix cannonTransform_;
    Matrix hatchTransform_;

    // Array holding all the bone transform matrices for the entire model.
    std::vector<Matrix> boneTransforms_;

    // NOXNA: rest-pose translation for every non-root tank.fbx part, read directly
    // from tank.fbx's own Lcl Translation node properties and composed through the
    // real (nested) parent chain -- see Load()'s own comment above. cna's
    // .model.json reader has no field to express this yet (DEFERRED.md item #6), so
    // it's applied here in C++ immediately after loading, exactly matching what
    // `leftBackWheelBone.Transform` etc. already held in real XNA's
    // content-pipeline-built Model. Every rotation/PreRotation in this asset is
    // zero (confirmed by direct read of tank.fbx), so each value below is a plain
    // translation, not a general matrix.
    void ApplyRestTransforms() {
        auto set = [&](const char* name, float x, float y, float z) {
            tankModel_->getBonesProperty()[name]->setTransformProperty(Matrix::CreateTranslation(x, y, z));
        };
        // r_engine_geo / l_engine_geo: direct children of tank_geo (Root); their own
        // Lcl Translation, unmodified.
        set("r_engine_geo",       -139.500504f, 163.812592f,  -10.956600f);
        set("l_engine_geo",        139.500504f, 163.812592f,  -10.956600f);
        // turret_geo: direct child of tank_geo (Root); its own Lcl Translation, unmodified.
        set("turret_geo",             0.000000f, 231.753983f,  -35.594997f);
        // *_back_wheel_geo, *_steer_geo: children of *_engine_geo -- folded with the
        // engine node's own translation above.
        set("r_back_wheel_geo",   -274.406403f, 104.886078f, -245.229645f);
        set("r_steer_geo",        -171.251114f, 185.973343f,  240.528397f);
        set("l_back_wheel_geo",    274.406403f, 104.886078f, -245.229645f);
        set("l_steer_geo",         171.251114f, 185.973343f,  240.528397f);
        // *_front_wheel_geo: children of *_steer_geo -- folded through both the steer
        // node and the engine node above it.
        set("r_front_wheel_geo",   -86.037399f,  72.994499f,  239.395996f);
        set("l_front_wheel_geo",    86.037399f,  72.994499f,  239.395996f);
        // canon_geo, hatch_geo: children of turret_geo -- folded with the turret
        // node's own translation above.
        set("canon_geo",              0.000000f, 336.395996f,   67.148903f);
        set("hatch_geo",             62.984013f, 357.659149f,  -79.181099f);
        // tank_geo (the tank body mesh) is intentionally left at Identity -- its own
        // real Lcl Translation is (0,0,0) with zero rotation, matching.
    }
};

} // namespace SimpleAnimation
