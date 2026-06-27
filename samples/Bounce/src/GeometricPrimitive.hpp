#pragma once
#include <memory>
#include <vector>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

namespace Bounce {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class GeometricPrimitive {
    std::vector<VertexPositionNormalTexture> vertices_;
    std::vector<uint16_t>                   indices_;

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer>  indexBuffer_;
    std::unique_ptr<BasicEffect>  basicEffect_;
    std::unique_ptr<BasicEffect>  basicEffectShadow_;

protected:
    void AddVertex(Vector3 position, Vector3 normal) {
        vertices_.push_back(VertexPositionNormalTexture(position, normal, Vector2(0.0f, 0.0f)));
    }

    void AddIndex(int index) {
        indices_.push_back(static_cast<uint16_t>(index));
    }

    int CurrentVertex() const { return (int)vertices_.size(); }

    void InitializePrimitive(GraphicsDevice& graphicsDevice) {
        vertexBuffer_ = std::make_unique<VertexBuffer>(
            graphicsDevice,
            VertexPositionNormalTexture::getVertexDeclarationStatic(),
            (int)vertices_.size(),
            BufferUsage::None);
        vertexBuffer_->SetData(vertices_.data(), (int)vertices_.size());

        indexBuffer_ = std::make_unique<IndexBuffer>(
            graphicsDevice, IndexElementSize::SixteenBits,
            (int)indices_.size(), BufferUsage::None);
        indexBuffer_->SetData(indices_.data(), (int)indices_.size());

        basicEffect_ = std::make_unique<BasicEffect>(graphicsDevice);
        basicEffect_->setLightingEnabledProperty(true);
        basicEffect_->getDirectionalLight0Property().setEnabledProperty(true);
        basicEffect_->getDirectionalLight0Property().setDiffuseColorProperty(Vector3::One);
        Vector3 lightDir(0.25f, -1.0f, -1.0f);
        lightDir.Normalize();
        basicEffect_->getDirectionalLight0Property().setDirectionProperty(lightDir);
        basicEffect_->getDirectionalLight0Property().setSpecularColorProperty(Vector3::One);
        basicEffect_->setSpecularColorProperty(Vector3::One);
        basicEffect_->setSpecularPowerProperty(32.0f);
        basicEffect_->setPreferPerPixelLightingProperty(false);

        basicEffectShadow_ = std::make_unique<BasicEffect>(graphicsDevice);
        basicEffectShadow_->setLightingEnabledProperty(false);
        basicEffectShadow_->setPreferPerPixelLightingProperty(false);
    }

public:
    virtual ~GeometricPrimitive() = default;

    void Draw(BasicEffect& effect) {
        GraphicsDevice& device = effect.getGraphicsDeviceInternal();
        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());
        for (auto& pass : effect.getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            int primitiveCount = (int)indices_.size() / 3;
            device.DrawIndexedPrimitives(
                PrimitiveType::TriangleList, 0, 0,
                (int)vertices_.size(), 0, primitiveCount);
        }
    }

    void Draw(Matrix world, Matrix view, Matrix projection, Color color, bool drawShadow) {
        BasicEffect& effect = drawShadow ? *basicEffectShadow_ : *basicEffect_;
        effect.setWorldProperty(world);
        effect.setViewProperty(view);
        effect.setProjectionProperty(projection);
        effect.setDiffuseColorProperty(color.ToVector3());
        effect.setAlphaProperty(color.getAProperty() / 255.0f);
        Draw(effect);
    }
};

} // namespace Bounce
