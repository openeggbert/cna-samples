#pragma once

#include <vector>
#include <stdexcept>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

namespace PrimitivesSample
{
    using namespace Microsoft::Xna::Framework;
    using namespace Microsoft::Xna::Framework::Graphics;

    class PrimitiveBatch
    {
        static constexpr int DefaultBufferSize = 500;

        std::vector<VertexPositionColor> vertices;
        int positionInBuffer = 0;

        BasicEffect basicEffect;
        GraphicsDevice& device;

        PrimitiveType primitiveType = PrimitiveType::TriangleList;
        int numVertsPerPrimitive = 3;
        bool hasBegun = false;

        static int NumVertsPerPrimitive(PrimitiveType pt)
        {
            switch (pt)
            {
                case PrimitiveType::LineList:     return 2;
                case PrimitiveType::TriangleList: return 3;
                default:
                    throw std::invalid_argument("primitive is not valid");
            }
        }

        void Flush()
        {
            if (!hasBegun)
                throw std::logic_error("Begin must be called before Flush.");

            if (positionInBuffer == 0)
                return;

            int primitiveCount = positionInBuffer / numVertsPerPrimitive;
            device.DrawUserPrimitives(primitiveType, vertices.data(), 0, primitiveCount);
            positionInBuffer = 0;
        }

    public:
        explicit PrimitiveBatch(GraphicsDevice& graphicsDevice)
            : device(graphicsDevice)
            , basicEffect(graphicsDevice)
        {
            vertices.resize(DefaultBufferSize);
            basicEffect.VertexColorEnabled = true;

            const auto& vp = graphicsDevice.getViewportProperty();
            basicEffect.Projection = Matrix::CreateOrthographicOffCenter(
                0.0f, static_cast<float>(vp.getWidthProperty()),
                static_cast<float>(vp.getHeightProperty()), 0.0f,
                0.0f, 1.0f);
        }

        void Begin(PrimitiveType pt)
        {
            if (hasBegun)
                throw std::logic_error("End must be called before Begin can be called again.");

            if (pt == PrimitiveType::LineStrip || pt == PrimitiveType::TriangleStrip)
                throw std::logic_error("PrimitiveType not supported by PrimitiveBatch.");

            primitiveType = pt;
            numVertsPerPrimitive = NumVertsPerPrimitive(pt);

            basicEffect.CurrentTechnique().Passes()[0].Apply();
            hasBegun = true;
        }

        void AddVertex(Vector2 vertex, Color color)
        {
            if (!hasBegun)
                throw std::logic_error("Begin must be called before AddVertex.");

            bool newPrimitive = (positionInBuffer % numVertsPerPrimitive) == 0;
            if (newPrimitive && (positionInBuffer + numVertsPerPrimitive) >= static_cast<int>(vertices.size()))
                Flush();

            vertices[positionInBuffer].Position = Vector3(vertex.X, vertex.Y, 0.0f);
            vertices[positionInBuffer].Color    = color;
            positionInBuffer++;
        }

        void End()
        {
            if (!hasBegun)
                throw std::logic_error("Begin must be called before End.");

            Flush();
            hasBegun = false;
        }
    };
}
