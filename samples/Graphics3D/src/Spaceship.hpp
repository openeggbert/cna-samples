#pragma once

#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Content/ContentManager.hpp>
#include <Microsoft/Xna/Framework/Graphics/Model.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelBone.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelEffectCollection.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/DirectionalLight.hpp>

#include <array>
#include <optional>
#include <vector>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace Graphics3DSample {

// Helper class for drawing the spaceship model with configurable per-frame
// lighting/texture state, matching the C# original's own Spaceship class.
class Spaceship {
public:
    Matrix Projection;
    Matrix Rotation;
    Matrix View;
    std::array<bool, 3> Lights = { true, true, true };
    bool IsTextureEnabled = false;
    bool IsPerPixelLightingEnabled = false;

    void Load(Content::ContentManager& content) {
        spaceshipModel_.emplace(content.Load<Model>("Models/spaceship"));
        boneTransforms_.assign(spaceshipModel_->getBonesProperty().getCountProperty(),
                                Matrix::getIdentityProperty());
    }

    void Draw() {
        spaceshipModel_->getRootProperty()->setTransformProperty(Rotation);

        spaceshipModel_->CopyAbsoluteBoneTransformsTo(boneTransforms_);

        for (ModelMesh* mesh : spaceshipModel_->getMeshesProperty()) {
            for (Effect* effect : mesh->getEffectsPropertyMutable()) {
                auto* basicEffect = static_cast<BasicEffect*>(effect);

                // A mesh with no explicit parent bone (CNA's simple .model.json
                // reader never assigns one — DEFERRED.md item #6) is attached to
                // the root bone, matching Model::Draw()'s own fallback.
                int boneIndex = mesh->getParentBoneProperty() ? mesh->getParentBoneProperty()->getIndexProperty() : 0;

                basicEffect->setWorldProperty(boneTransforms_[boneIndex]);
                basicEffect->setViewProperty(View);
                basicEffect->setProjectionProperty(Projection);

                SetEffectLights(*basicEffect);
                basicEffect->setPreferPerPixelLightingProperty(IsPerPixelLightingEnabled);
                basicEffect->setTextureEnabledProperty(IsTextureEnabled);
                if (texture_.has_value()) basicEffect->setTextureProperty(&*texture_);
            }

            mesh->Draw();
        }
    }

    void SetTexture(Texture2D texture) { texture_.emplace(std::move(texture)); }

private:
    std::optional<Model> spaceshipModel_;
    std::vector<Matrix> boneTransforms_;
    std::optional<Texture2D> texture_;

    void SetEffectLights(BasicEffect& effect) {
        effect.setAlphaProperty(1.0f);
        effect.setDiffuseColorProperty(Vector3(0.75f, 0.75f, 0.75f));
        effect.setSpecularColorProperty(Vector3(0.25f, 0.25f, 0.25f));
        effect.setSpecularPowerProperty(5.0f);
        effect.setAmbientLightColorProperty(Vector3(0.75f, 0.75f, 0.75f));

        DirectionalLight& light0 = effect.getDirectionalLight0Property();
        light0.setEnabledProperty(Lights[0]);
        light0.setDiffuseColorProperty(Vector3::One);
        light0.setDirectionProperty(Vector3::Normalize(Vector3(1, -1, 0)));
        light0.setSpecularColorProperty(Vector3::One);

        DirectionalLight& light1 = effect.getDirectionalLight1Property();
        light1.setEnabledProperty(Lights[1]);
        light1.setDiffuseColorProperty(Vector3(0.5f, 0.5f, 0.5f));
        light1.setDirectionProperty(Vector3::Normalize(Vector3(-1, -1, 0)));
        light1.setSpecularColorProperty(Vector3(1.0f, 1.0f, 1.0f));

        DirectionalLight& light2 = effect.getDirectionalLight2Property();
        light2.setEnabledProperty(Lights[2]);
        light2.setDiffuseColorProperty(Vector3(0.3f, 0.3f, 0.3f));
        light2.setDirectionProperty(Vector3::Normalize(Vector3(-1, -1, -1)));
        light2.setSpecularColorProperty(Vector3(0.3f, 0.3f, 0.3f));

        effect.setLightingEnabledProperty(true);
    }
};

} // namespace Graphics3DSample
