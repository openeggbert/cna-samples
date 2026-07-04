#pragma once

// ProceduralPrimitive.hpp — C++ port of ProceduralPrimitive.cs (XNA 4.0
// SoccerPitch sample). Generic base for procedurally-built geometry: a vertex
// buffer, an index buffer, plus draw methods for BasicEffect, AlphaTestEffect,
// and DualTextureEffect. Port of the original's `ProceduralPrimitive<T>`.

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Graphics/AlphaTestEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DualTextureEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexDeclaration.hpp"

namespace SoccerPitch {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

// Generic base for a procedurally-built primitive. T must be one of the vertex
// types VertexBuffer::SetData understands (this sample only ever instantiates
// it with VertexPositionNormalTexture — see PlanePrimitiveDualTextured.hpp for
// why the original's second, dual-UV vertex type isn't used here).
template <typename T>
class ProceduralPrimitive {
protected:
    std::vector<T> vertices_;
    std::vector<uint16_t> indices_;

    void AddIndex(int index) {
        if (index > 65535)
            throw std::out_of_range("index");
        indices_.push_back((uint16_t)index);
    }

    void AddVertex(const T& vertex) { vertices_.push_back(vertex); }

    [[nodiscard]] int CurrentVertex() const { return (int)vertices_.size(); }

    void InitializePrimitive(GraphicsDevice& device) {
        vertexBuffer_ = std::make_unique<VertexBuffer>(device, (int)vertices_.size());
        vertexBuffer_->SetData(vertices_.data(), (int)vertices_.size());

        indexBuffer_ = std::make_unique<IndexBuffer>(device, (int)indices_.size());
        indexBuffer_->SetData(indices_.data(), (int)indices_.size());

        vertices_.clear();
        vertices_.shrink_to_fit();
        indices_.clear();
        indices_.shrink_to_fit();
    }

public:
    virtual ~ProceduralPrimitive() = default;
    ProceduralPrimitive() = default;
    ProceduralPrimitive(ProceduralPrimitive&&) = default;
    ProceduralPrimitive& operator=(ProceduralPrimitive&&) = default;

    // Draws using a caller-supplied effect. Does not set any renderstates.
    void Draw(Effect& effect) {
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

    // Draws using a BasicEffect, setting sensible renderstates first.
    void Draw(BasicEffect& basicEffect, const Matrix& world, const Matrix& view, const Matrix& projection,
              Color color) {
        basicEffect.World = world;
        basicEffect.View = view;
        basicEffect.Projection = projection;
        basicEffect.setDiffuseColorProperty(
            Vector3(color.getRProperty() / 255.0f, color.getGProperty() / 255.0f, color.getBProperty() / 255.0f));
        basicEffect.setAlphaProperty(color.getAProperty() / 255.0f);

        GraphicsDevice& device = basicEffect.getGraphicsDeviceInternal();
        if (color.getAProperty() < 255) {
            device.setDepthStencilStateProperty(DepthStencilState::None);
            device.setBlendStateProperty(BlendState::Additive);
        } else {
            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setBlendStateProperty(BlendState::Opaque);
        }

        Draw(basicEffect);
    }

    // Draws using an AlphaTestEffect, setting sensible renderstates first.
    void DrawAlphaTest(AlphaTestEffect& atEffect, const Matrix& world, const Matrix& view, const Matrix& projection,
                        Color color) {
        atEffect.setWorldProperty(world);
        atEffect.setViewProperty(view);
        atEffect.setProjectionProperty(projection);
        atEffect.setDiffuseColorProperty(
            Vector3(color.getRProperty() / 255.0f, color.getGProperty() / 255.0f, color.getBProperty() / 255.0f));
        atEffect.setAlphaProperty(color.getAProperty() / 255.0f);

        GraphicsDevice& device = atEffect.getGraphicsDeviceInternal();
        if (color.getAProperty() < 255) {
            device.setDepthStencilStateProperty(DepthStencilState::None);
            device.setBlendStateProperty(BlendState::Additive);
        } else {
            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setBlendStateProperty(BlendState::Opaque);
        }

        Draw(atEffect);
    }

    // Draws using a DualTextureEffect, setting sensible renderstates first.
    void DrawDualTextured(DualTextureEffect& dtEffect, const Matrix& world, const Matrix& view,
                           const Matrix& projection, Color color) {
        dtEffect.setWorldProperty(world);
        dtEffect.setViewProperty(view);
        dtEffect.setProjectionProperty(projection);
        dtEffect.setDiffuseColorProperty(
            Vector3(color.getRProperty() / 255.0f, color.getGProperty() / 255.0f, color.getBProperty() / 255.0f));
        dtEffect.setAlphaProperty(color.getAProperty() / 255.0f);

        GraphicsDevice& device = dtEffect.getGraphicsDeviceInternal();
        if (color.getAProperty() < 255) {
            device.setDepthStencilStateProperty(DepthStencilState::None);
            device.setBlendStateProperty(BlendState::Additive);
        } else {
            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setBlendStateProperty(BlendState::Opaque);
        }

        Draw(dtEffect);
    }

private:
    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;
};

} // namespace SoccerPitch
