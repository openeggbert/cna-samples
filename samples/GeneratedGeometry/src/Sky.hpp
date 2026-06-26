#pragma once

#include <cmath>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionTexture.hpp"

namespace GeneratedGeometrySample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Content;
    using namespace Microsoft::Xna::Framework::Graphics;

    class Sky
    {
        static constexpr float cylinderSize       = 100.0f;
        static constexpr int   cylinderSegments   = 32;
        static constexpr float texCoordTop        = 0.1f;
        static constexpr float texCoordBottom     = 0.9f;

        std::unique_ptr<BasicEffect>              effect_;
        Texture2D                                 skyTexture_;
        std::vector<VertexPositionTexture>        skyVerts_;

        void BuildCylinder()
        {
            // Pre-compute ring positions (same as SkyProcessor.cs)
            std::vector<Vector3> top(cylinderSegments);
            std::vector<Vector3> bottom(cylinderSegments);

            for (int i = 0; i < cylinderSegments; i++) {
                float angle = MathHelper::TwoPi * i / cylinderSegments;
                float x = std::cos(angle) * cylinderSize;
                float z = std::sin(angle) * cylinderSize;
                top[i]    = Vector3(x,  cylinderSize, z);
                bottom[i] = Vector3(x, -cylinderSize, z);
            }

            Vector3 centerTop   (0.0f,  cylinderSize * 2.0f, 0.0f);
            Vector3 centerBottom(0.0f, -cylinderSize * 2.0f, 0.0f);

            skyVerts_.clear();
            skyVerts_.reserve(cylinderSegments * 4 * 3);

            for (int i = 0; i < cylinderSegments; i++) {
                int j = (i + 1) % cylinderSegments;

                float u1 = static_cast<float>(i)     / cylinderSegments;
                float u2 = static_cast<float>(i + 1) / cylinderSegments;

                // Side quad (2 triangles)
                skyVerts_.push_back({top[i],    {u1, texCoordTop}});
                skyVerts_.push_back({top[j],    {u2, texCoordTop}});
                skyVerts_.push_back({bottom[i], {u1, texCoordBottom}});

                skyVerts_.push_back({top[j],    {u2, texCoordTop}});
                skyVerts_.push_back({bottom[j], {u2, texCoordBottom}});
                skyVerts_.push_back({bottom[i], {u1, texCoordBottom}});

                // Top cap (fan inward)
                skyVerts_.push_back({centerTop, {u1, 0.0f}});
                skyVerts_.push_back({top[j],    {u2, texCoordTop}});
                skyVerts_.push_back({top[i],    {u1, texCoordTop}});

                // Bottom cap (fan inward)
                skyVerts_.push_back({centerBottom, {u1, 1.0f}});
                skyVerts_.push_back({bottom[i],    {u1, texCoordBottom}});
                skyVerts_.push_back({bottom[j],    {u2, texCoordBottom}});
            }
        }

    public:
        void LoadContent(ContentManager& content, GraphicsDevice& device)
        {
            skyTexture_ = content.Load<Texture2D>("sky.bmp");

            BuildCylinder();

            effect_ = std::make_unique<BasicEffect>(device);
            effect_->setWorldProperty(Matrix::getIdentityProperty());
            effect_->setTextureEnabledProperty(true);
            effect_->setTextureProperty(&skyTexture_);
            effect_->setLightingEnabledProperty(false);
            effect_->VertexColorEnabled = false;
        }

        void Draw(Matrix view, Matrix projection)
        {
            // Sky should not move with the camera — zero out view translation
            view.setTranslationProperty(Vector3::Zero);

            // Force sky vertices to the far clip plane: z = w in clip space
            projection.M13 = projection.M14;
            projection.M23 = projection.M24;
            projection.M33 = projection.M34;
            projection.M43 = projection.M44;

            effect_->setViewProperty(view);
            effect_->setProjectionProperty(projection);

            auto& device = effect_->getGraphicsDeviceInternal();
            device.setDepthStencilStateProperty(DepthStencilState::DepthRead);
            device.setBlendStateProperty(BlendState::Opaque);
            device.setRasterizerStateProperty(RasterizerState::CullNone);

            for (auto& pass : effect_->getCurrentTechniqueProperty()->getPassesProperty()) {
                pass.Apply();
                device.DrawUserPrimitives(PrimitiveType::TriangleList,
                    skyVerts_.data(), 0,
                    static_cast<int>(skyVerts_.size()) / 3);
            }

            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        }
    };

} // namespace GeneratedGeometrySample
