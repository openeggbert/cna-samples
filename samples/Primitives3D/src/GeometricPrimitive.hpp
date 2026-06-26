#pragma once

// GeometricPrimitive.hpp — C++ port of GeometricPrimitive.cs (XNA 4.0 Primitives3D sample).
//
// Adaptation note: XNA uses VertexPositionNormal + lit BasicEffect.
// CNA currently only supports VertexPositionColor, so this port stores position
// + white color vertices and applies the draw color via DiffuseColor at Draw() time.
// Lighting is therefore flat/absent until CNA adds normal-mapped 3D shader support.

#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

namespace Primitives3D
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;

    class GeometricPrimitive
    {
        std::vector<Vector3>   positions;
        std::vector<Vector3>   normals;
        std::vector<uint16_t>  indices;

        std::unique_ptr<VertexBuffer> vertexBuffer;
        std::unique_ptr<IndexBuffer>  indexBuffer;
        std::unique_ptr<BasicEffect>  basicEffect;

    protected:
        void AddVertex(Vector3 position, Vector3 normal)
        {
            positions.push_back(position);
            normals.push_back(normal);
        }

        void AddIndex(int index)
        {
            if (index > 65535)
                throw std::out_of_range("index");
            indices.push_back(static_cast<uint16_t>(index));
        }

        [[nodiscard]] int CurrentVertex() const
        {
            return static_cast<int>(positions.size());
        }

        void InitializePrimitive(GraphicsDevice& device)
        {
            std::vector<VertexPositionColor> verts(positions.size());
            for (size_t i = 0; i < positions.size(); ++i)
            {
                verts[i].Position = positions[i];
                verts[i].Color    = Color::White;
            }

            vertexBuffer = std::make_unique<VertexBuffer>(
                device,
                static_cast<int>(verts.size()));
            vertexBuffer->SetData(verts.data(), static_cast<int>(verts.size()));

            indexBuffer = std::make_unique<IndexBuffer>(
                device,
                static_cast<int>(indices.size()));
            indexBuffer->SetData(indices.data(), static_cast<int>(indices.size()));

            basicEffect = std::make_unique<BasicEffect>(device);

            positions.clear();
            normals.clear();
            indices.shrink_to_fit();
        }

    public:
        virtual ~GeometricPrimitive() = default;

        void Draw(const Matrix& world, const Matrix& view, const Matrix& projection, Color color)
        {
            GraphicsDevice& device = basicEffect->getGraphicsDeviceInternal();

            basicEffect->World      = world;
            basicEffect->View       = view;
            basicEffect->Projection = projection;

            basicEffect->setDiffuseColorProperty(
                Vector3(color.getRProperty() / 255.0f,
                        color.getGProperty() / 255.0f,
                        color.getBProperty() / 255.0f));
            basicEffect->setAlphaProperty(color.getAProperty() / 255.0f);

            device.SetDepthTestEnabled(true);
            if (color.getAProperty() < 255)
            {
                device.SetBlendEnabled(true);
                device.SetDepthWriteEnabled(false);
            }
            else
            {
                device.SetBlendEnabled(false);
                device.SetDepthWriteEnabled(true);
            }

            Draw(*basicEffect);

            device.SetBlendEnabled(false);
            device.SetDepthWriteEnabled(true);
        }

        void Draw(Effect& effect)
        {
            GraphicsDevice& device = effect.getGraphicsDeviceInternal();

            device.SetVertexBuffer(vertexBuffer.get());
            device.setIndicesProperty(indexBuffer.get());

            int primitiveCount = indexBuffer->getIndexCountProperty() / 3;

            for (auto& pass : effect.getCurrentTechniqueProperty()->getPassesProperty())
            {
                pass.Apply();
                device.DrawIndexedPrimitives(
                    PrimitiveType::TriangleList,
                    0, 0,
                    vertexBuffer->getVertexCountProperty(),
                    0, primitiveCount);
            }
        }
    };
}
