#pragma once

// GeometricPrimitive.hpp — C++ port of Primitives/GeometricPrimitive.cs (XNA
// 4.0 PerformanceMeasuring sample; itself borrowed from the Primitives3D
// sample in the original, as its doc comment says).
//
// Adaptation note: XNA uses VertexPositionNormal + lit BasicEffect. CNA
// currently only supports VertexPositionColor, so this port stores position +
// white color vertices and applies the draw color via DiffuseColor at Draw()
// time. Lighting is therefore flat/absent — same simplification already
// established in samples/Primitives3D (DEFERRED.md item 5). See missing.md.

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

namespace PerformanceMeasuring {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

// Base class for simple geometric primitive models: a vertex buffer, an index
// buffer, plus methods for drawing. Port of Primitives/GeometricPrimitive.cs.
class GeometricPrimitive {
    std::vector<Vector3> positions_;
    std::vector<uint16_t> indices_;

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;
    std::unique_ptr<BasicEffect> basicEffect_;

protected:
    void AddVertex(Vector3 position, Vector3 /*normal*/) { positions_.push_back(position); }

    void AddIndex(int index) {
        if (index > 65535)
            throw std::out_of_range("index");
        indices_.push_back((uint16_t)index);
    }

    [[nodiscard]] int CurrentVertex() const { return (int)positions_.size(); }

    void InitializePrimitive(GraphicsDevice& device) {
        std::vector<VertexPositionColor> verts(positions_.size());
        for (size_t i = 0; i < positions_.size(); ++i) {
            verts[i].Position = positions_[i];
            verts[i].Color = Color::White;
        }

        vertexBuffer_ = std::make_unique<VertexBuffer>(device, (int)verts.size());
        vertexBuffer_->SetData(verts.data(), (int)verts.size());

        indexBuffer_ = std::make_unique<IndexBuffer>(device, (int)indices_.size());
        indexBuffer_->SetData(indices_.data(), (int)indices_.size());

        basicEffect_ = std::make_unique<BasicEffect>(device);

        positions_.clear();
        positions_.shrink_to_fit();
        indices_.clear();
        indices_.shrink_to_fit();
    }

public:
    GeometricPrimitive() = default;
    GeometricPrimitive(GeometricPrimitive&&) = default;
    GeometricPrimitive& operator=(GeometricPrimitive&&) = default;
    virtual ~GeometricPrimitive() = default;

    // Draws the primitive with a BasicEffect using default renderstates.
    void Draw(const Matrix& world, const Matrix& view, const Matrix& projection, Color color) {
        GraphicsDevice& device = basicEffect_->getGraphicsDeviceInternal();

        basicEffect_->World = world;
        basicEffect_->View = view;
        basicEffect_->Projection = projection;

        basicEffect_->setDiffuseColorProperty(Vector3(color.getRProperty() / 255.0f, color.getGProperty() / 255.0f,
                                                        color.getBProperty() / 255.0f));
        basicEffect_->setAlphaProperty(color.getAProperty() / 255.0f);

        device.setDepthStencilStateProperty(DepthStencilState::Default);
        device.setBlendStateProperty(color.getAProperty() < 255 ? BlendState::AlphaBlend : BlendState::Opaque);

        Draw(*basicEffect_);
    }

    // Draws the primitive with a caller-supplied effect. Unlike the overload
    // above, this does not set any renderstates.
    void Draw(Effect& effect) {
        GraphicsDevice& device = effect.getGraphicsDeviceInternal();

        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());

        int primitiveCount = indexBuffer_->getIndexCountProperty() / 3;

        for (auto& pass : effect.getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexBuffer_->getVertexCountProperty(),
                                          0, primitiveCount);
        }
    }
};

} // namespace PerformanceMeasuring
