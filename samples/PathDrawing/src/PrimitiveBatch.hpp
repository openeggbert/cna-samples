#pragma once
#include <stdexcept>
#include <vector>
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

namespace PathDrawing {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class PrimitiveBatch {
    static constexpr int DefaultBufferSize = 500;

    std::vector<VertexPositionColor> vertices_;
    int positionInBuffer_ = 0;

    BasicEffect basicEffect_;
    GraphicsDevice& device_;

    PrimitiveType primitiveType_ = PrimitiveType::LineList;
    int numVertsPerPrimitive_ = 2;
    bool hasBegun_ = false;

public:
    explicit PrimitiveBatch(GraphicsDevice& graphicsDevice)
        : device_(graphicsDevice), basicEffect_(graphicsDevice)
    {
        vertices_.resize(DefaultBufferSize);

        basicEffect_.VertexColorEnabled = true;

        float w = (float)graphicsDevice.getViewportProperty().getWidthProperty();
        float h = (float)graphicsDevice.getViewportProperty().getHeightProperty();
        basicEffect_.setProjectionProperty(
            Matrix::CreateOrthographicOffCenter(0, w, h, 0, 0, 1));
    }

    void Begin(PrimitiveType primitiveType) {
        if (hasBegun_)
            throw std::runtime_error("End must be called before Begin.");
        if (primitiveType == PrimitiveType::LineStrip ||
            primitiveType == PrimitiveType::TriangleStrip)
            throw std::runtime_error("LineStrip and TriangleStrip are not supported.");

        primitiveType_ = primitiveType;
        numVertsPerPrimitive_ = (primitiveType == PrimitiveType::TriangleList) ? 3 : 2;

        basicEffect_.getCurrentTechniqueProperty()->getPassesProperty()[0].Apply();
        hasBegun_ = true;
    }

    void AddVertex(Vector2 vertex, Color color) {
        if (!hasBegun_)
            throw std::runtime_error("Begin must be called before AddVertex.");

        bool newPrimitive = ((positionInBuffer_ % numVertsPerPrimitive_) == 0);
        if (newPrimitive && (positionInBuffer_ + numVertsPerPrimitive_) >= (int)vertices_.size())
            Flush();

        vertices_[positionInBuffer_].Position = Vector3(vertex.X, vertex.Y, 0.0f);
        vertices_[positionInBuffer_].Color    = color;
        positionInBuffer_++;
    }

    void End() {
        if (!hasBegun_)
            throw std::runtime_error("Begin must be called before End.");
        Flush();
        hasBegun_ = false;
    }

private:
    void Flush() {
        int primitiveCount = positionInBuffer_ / numVertsPerPrimitive_;
        if (primitiveCount == 0) return;

        device_.DrawUserPrimitives(primitiveType_, vertices_.data(), 0, primitiveCount);
        positionInBuffer_ = 0;
    }
};

} // namespace PathDrawing
