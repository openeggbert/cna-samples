#pragma once

#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

namespace TexturesAndColorsSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;

    class SampleGrid
    {
        int   gridSize_       = 16;
        float gridScale_      = 32.0f;
        Color gridColor_      = Color::White;
        int   primitiveCount_ = 0;

        std::unique_ptr<VertexBuffer> vertexBuffer_;
        std::unique_ptr<BasicEffect>  effect_;
        GraphicsDevice*               device_ = nullptr;

    public:
        Color  GridColor      = Color::White;
        int    GridSize       = 16;
        float  GridScale      = 32.0f;
        Matrix ProjectionMatrix = Matrix::getIdentityProperty();
        Matrix WorldMatrix      = Matrix::getIdentityProperty();
        Matrix ViewMatrix       = Matrix::getIdentityProperty();

        SampleGrid() = default;

        void LoadGraphicsContent(GraphicsDevice& device)
        {
            device_ = &device;
            effect_ = std::make_unique<BasicEffect>(device);

            int gridSize1 = GridSize + 1;
            primitiveCount_ = gridSize1 * 2;
            int vertexCount = primitiveCount_ * 2;

            std::vector<VertexPositionColor> vertices(vertexCount);

            float length     = static_cast<float>(GridSize) * GridScale;
            float halfLength = length * 0.5f;

            int index = 0;
            for (int i = 0; i < gridSize1; ++i)
            {
                float offset = i * GridScale - halfLength;
                vertices[index++] = VertexPositionColor(
                    Vector3(-halfLength, 0.0f, offset), GridColor);
                vertices[index++] = VertexPositionColor(
                    Vector3( halfLength, 0.0f, offset), GridColor);
                vertices[index++] = VertexPositionColor(
                    Vector3(offset, 0.0f, -halfLength), GridColor);
                vertices[index++] = VertexPositionColor(
                    Vector3(offset, 0.0f,  halfLength), GridColor);
            }

            vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount);
            vertexBuffer_->SetData(vertices.data(), vertexCount);
        }

        void Draw()
        {
            if (!device_ || !effect_ || !vertexBuffer_)
                return;

            effect_->World      = WorldMatrix;
            effect_->View       = ViewMatrix;
            effect_->Projection = ProjectionMatrix;
            effect_->VertexColorEnabled = true;
            effect_->setLightingEnabledProperty(false);

            device_->SetVertexBuffer(vertexBuffer_.get());

            for (auto& pass : effect_->getCurrentTechniqueProperty()->getPassesProperty())
            {
                pass.Apply();
                device_->DrawPrimitives(PrimitiveType::LineList, 0, primitiveCount_);
            }
        }
    };
}
