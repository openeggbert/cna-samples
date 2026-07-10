#pragma once

// Port of GeometricPrimitive.cs (XNA 4.0 TiltPerspective sample) -- base class
// for procedurally-generated primitives (only SpherePrimitive is used by this
// sample). See SpherePrimitive.hpp for the concrete geometry generator.
//
// Vertex format note (DEFERRED.md item #5's still-open remainder): the C#
// original defines its own texture-less VertexPositionNormal struct
// (VertexPositionNormal.cs, its own IVertexType) for these procedural
// primitives, since they have no UV data at all. CNA has no texture-less
// normal-lit vertex format. Per item #5's own recommended workaround --
// already applied the same way by this repo's Primitives3D sample's own
// still-open "flat shading" note (samples/Primitives3D/missing.md) -- this
// port assigns a dummy, unused Vector2(0,0) texture coordinate to every
// vertex and uses the already-proven, already-working VertexPositionNormalTexture
// + BasicEffect lit path instead of inventing a new CNA vertex type.
// VertexPositionNormal.cs itself is not ported -- there is nothing left for
// it to do once VertexPositionNormalTexture stands in for it.

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

namespace TiltPerspectiveSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class GeometricPrimitive {
public:
    virtual ~GeometricPrimitive() = default;

    // Matches the original's `LightDirection` setter -- overrides
    // basicEffect's DirectionalLight0.Direction. Called once per frame by
    // BallSimulation::Draw() before drawing any balls.
    void setLightDirectionProperty(Vector3 value) {
        value.Normalize();
        basicEffect_->getDirectionalLight0Property().setDirectionProperty(value);
    }

    // Draws the primitive using a caller-supplied effect, with no render
    // state changes (matches `Draw(BasicEffect effect)`).
    void Draw(BasicEffect& effect) {
        GraphicsDevice& device = effect.getGraphicsDeviceInternal();

        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());

        int primitiveCount = indexBuffer_->getIndexCountProperty() / 3;

        for (auto& pass : effect.getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0,
                                          vertexBuffer_->getVertexCountProperty(), 0, primitiveCount);
        }
    }

    // Draws the primitive with a full BasicEffect setup (world/view/projection,
    // diffuse color/alpha, and the depth/blend states needed for opaque vs.
    // translucent geometry). Matches `Draw(Matrix, Matrix, Matrix, Color, bool)`.
    void Draw(const Matrix& world, const Matrix& view, const Matrix& projection, Color color, bool drawShadow) {
        BasicEffect& drawBasicEffect = drawShadow ? *basicEffectShadow_ : *basicEffect_;

        drawBasicEffect.World = world;
        drawBasicEffect.View = view;
        drawBasicEffect.Projection = projection;
        drawBasicEffect.setDiffuseColorProperty(color.ToVector3());
        drawBasicEffect.setAlphaProperty(color.getAProperty() / 255.0f);

        GraphicsDevice& device = basicEffect_->getGraphicsDeviceInternal();

        if (color.getAProperty() < 255) {
            device.setDepthStencilStateProperty(DepthStencilState::DepthRead);
            device.setBlendStateProperty(BlendState::AlphaBlend);
        } else {
            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setBlendStateProperty(BlendState::Opaque);
        }

        Draw(drawBasicEffect);
    }

protected:
    void AddVertex(const Vector3& position, const Vector3& normal) {
        // NOXNA: dummy (0,0) texture coordinate -- see the file-level comment.
        vertices_.emplace_back(position, normal, Vector2(0.0f, 0.0f));
    }

    void AddIndex(int index) {
        if (index > 65535)
            throw std::out_of_range("index");
        indices_.push_back(static_cast<std::uint16_t>(index));
    }

    [[nodiscard]] int CurrentVertex() const { return static_cast<int>(vertices_.size()); }

    void InitializePrimitive(GraphicsDevice& device) {
        vertexBuffer_ = std::make_unique<VertexBuffer>(device, static_cast<int>(vertices_.size()));
        vertexBuffer_->SetData(vertices_.data(), static_cast<int>(vertices_.size()));

        indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices_.size()));
        indexBuffer_->SetData(indices_.data(), static_cast<int>(indices_.size()));

        basicEffect_ = std::make_unique<BasicEffect>(device);
        basicEffect_->setLightingEnabledProperty(true);

        basicEffect_->getDirectionalLight0Property().setEnabledProperty(true);
        Vector3 initialLightDirection(0.25f, -1.0f, -1.0f);
        initialLightDirection.Normalize();
        basicEffect_->getDirectionalLight0Property().setDirectionProperty(initialLightDirection);

        basicEffect_->setSpecularColorProperty(Vector3(1.0f, 1.0f, 1.0f));
        basicEffect_->getDirectionalLight0Property().setSpecularColorProperty(Vector3(1.0f, 1.0f, 1.0f));
        basicEffect_->setSpecularPowerProperty(32.0f);
        basicEffect_->setPreferPerPixelLightingProperty(false);

        basicEffectShadow_ = std::make_unique<BasicEffect>(device);
        basicEffectShadow_->setLightingEnabledProperty(false);
        basicEffectShadow_->setPreferPerPixelLightingProperty(false);

        // The CPU-side vertex/index lists have now been uploaded to the GPU;
        // the original clears its own `vertices`/`indices` Lists too once
        // InitializePrimitive() has run (implicitly, by never touching them
        // again). Kept here for parity, freeing the CPU-side copies.
        vertices_.clear();
        vertices_.shrink_to_fit();
        indices_.clear();
        indices_.shrink_to_fit();
    }

private:
    std::vector<VertexPositionNormalTexture> vertices_;
    std::vector<std::uint16_t> indices_;

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;
    std::unique_ptr<BasicEffect> basicEffect_;
    std::unique_ptr<BasicEffect> basicEffectShadow_;
};

} // namespace TiltPerspectiveSample
