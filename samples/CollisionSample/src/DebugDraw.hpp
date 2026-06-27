#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include "BoundingOrientedBox.hpp"
#include "TriangleTest.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Ray.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicIndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/DynamicVertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/SetDataOptions.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"

namespace CollisionSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class DebugDraw {
    static constexpr int MAX_VERTS   = 2000;
    static constexpr int MAX_INDICES = 2000;

    static constexpr uint16_t cubeIndices_[24] = {
        0,1, 1,2, 2,3, 3,0,  4,5, 5,6, 6,7, 7,4,  0,4, 1,5, 2,6, 3,7
    };

    BasicEffect           basicEffect_;
    DynamicVertexBuffer   vertexBuffer_;
    DynamicIndexBuffer    indexBuffer_;

    uint16_t           Indices[MAX_INDICES]{};
    VertexPositionColor Vertices[MAX_VERTS]{};
    int IndexCount  = 0;
    int VertexCount = 0;

public:
    explicit DebugDraw(GraphicsDevice& device)
        : basicEffect_(device)
        , vertexBuffer_(device, VertexPositionColor::getVertexDeclarationStatic(), MAX_VERTS, BufferUsage::WriteOnly)
        , indexBuffer_(device, IndexElementSize::SixteenBits, MAX_INDICES, BufferUsage::WriteOnly)
    {
        basicEffect_.setLightingEnabledProperty(false);
        basicEffect_.VertexColorEnabled = true;
        basicEffect_.setTextureEnabledProperty(false);
    }

    void Begin(Matrix view, Matrix projection) {
        basicEffect_.setWorldProperty(Matrix::getIdentityProperty());
        basicEffect_.setViewProperty(view);
        basicEffect_.setProjectionProperty(projection);
        VertexCount = 0;
        IndexCount  = 0;
    }

    void End() {
        FlushDrawing();
    }

    void DrawWireShape(const std::vector<Vector3>& positionArray, const uint16_t* indexArray, int indexCount, Color color) {
        int numVerts = (int)positionArray.size();
        if (Reserve(numVerts, indexCount)) {
            for (int i = 0; i < indexCount; i++)
                Indices[IndexCount++] = (uint16_t)(VertexCount + indexArray[i]);
            for (int i = 0; i < numVerts; i++)
                Vertices[VertexCount++] = VertexPositionColor(positionArray[i], color);
        }
    }

    void DrawWireGrid(Vector3 xAxis, Vector3 yAxis, Vector3 origin, int iXDivisions, int iYDivisions, Color color) {
        Vector3 pos = origin;
        Vector3 step = xAxis * (1.0f / iXDivisions);
        for (int i = 0; i <= iXDivisions; i++) {
            DrawLine(pos, pos + yAxis, color);
            pos = pos + step;
        }
        pos = origin;
        step = yAxis * (1.0f / iYDivisions);
        for (int i = 0; i <= iYDivisions; i++) {
            DrawLine(pos, pos + xAxis, color);
            pos = pos + step;
        }
    }

    void DrawWireFrustum(const BoundingFrustum& frustum, Color color) {
        DrawWireShape(frustum.GetCorners(), cubeIndices_, 24, color);
    }

    void DrawWireBox(const BoundingBox& box, Color color) {
        DrawWireShape(box.GetCorners(), cubeIndices_, 24, color);
    }

    void DrawWireBox(const BoundingOrientedBox& box, Color color) {
        DrawWireShape(box.GetCorners(), cubeIndices_, 24, color);
    }

    void DrawRing(Vector3 origin, Vector3 majorAxis, Vector3 minorAxis, Color color) {
        constexpr int RING_SEGMENTS = 32;
        constexpr float fAngleDelta = 2.0f * 3.14159265358979323846f / RING_SEGMENTS;
        if (Reserve(RING_SEGMENTS, RING_SEGMENTS * 2)) {
            for (int i = 0; i < RING_SEGMENTS; i++) {
                Indices[IndexCount++] = (uint16_t)(VertexCount + i);
                Indices[IndexCount++] = (uint16_t)(VertexCount + (i + 1) % RING_SEGMENTS);
            }
            float cosDelta = std::cos(fAngleDelta);
            float sinDelta = std::sin(fAngleDelta);
            float cosAcc = 1.0f, sinAcc = 0.0f;
            for (int i = 0; i < RING_SEGMENTS; i++) {
                Vector3 pos(
                    majorAxis.X * cosAcc + minorAxis.X * sinAcc + origin.X,
                    majorAxis.Y * cosAcc + minorAxis.Y * sinAcc + origin.Y,
                    majorAxis.Z * cosAcc + minorAxis.Z * sinAcc + origin.Z);
                Vertices[VertexCount++] = VertexPositionColor(pos, color);
                float newCos = cosAcc * cosDelta - sinAcc * sinDelta;
                float newSin = cosAcc * sinDelta + sinAcc * cosDelta;
                cosAcc = newCos;
                sinAcc = newSin;
            }
        }
    }

    void DrawWireSphere(const BoundingSphere& sphere, Color color) {
        Matrix view = basicEffect_.getWorldProperty() * basicEffect_.getViewProperty();
        view = Matrix::Transpose(view);
        DrawRing(sphere.Center, view.getRightProperty() * sphere.Radius, view.getUpProperty() * sphere.Radius, color);
    }

    void DrawRay(const Ray& ray, Color color, float length) {
        DrawLine(ray.Position, ray.Position + ray.Direction * length, color);
    }

    void DrawLine(Vector3 v0, Vector3 v1, Color color) {
        if (Reserve(2, 2)) {
            Indices[IndexCount++] = (uint16_t)VertexCount;
            Indices[IndexCount++] = (uint16_t)(VertexCount + 1);
            Vertices[VertexCount++] = VertexPositionColor(v0, color);
            Vertices[VertexCount++] = VertexPositionColor(v1, color);
        }
    }

    void DrawWireTriangle(Vector3 v0, Vector3 v1, Vector3 v2, Color color) {
        if (Reserve(3, 6)) {
            Indices[IndexCount++] = (uint16_t)(VertexCount + 0);
            Indices[IndexCount++] = (uint16_t)(VertexCount + 1);
            Indices[IndexCount++] = (uint16_t)(VertexCount + 1);
            Indices[IndexCount++] = (uint16_t)(VertexCount + 2);
            Indices[IndexCount++] = (uint16_t)(VertexCount + 2);
            Indices[IndexCount++] = (uint16_t)(VertexCount + 0);
            Vertices[VertexCount++] = VertexPositionColor(v0, color);
            Vertices[VertexCount++] = VertexPositionColor(v1, color);
            Vertices[VertexCount++] = VertexPositionColor(v2, color);
        }
    }

    void DrawWireTriangle(const Triangle& t, Color color) {
        DrawWireTriangle(t.V0, t.V1, t.V2, color);
    }

private:
    void FlushDrawing() {
        if (IndexCount > 0) {
            vertexBuffer_.SetData(Vertices, 0, VertexCount, SetDataOptions::Discard);
            indexBuffer_.SetData(Indices, 0, IndexCount, SetDataOptions::Discard);

            GraphicsDevice& device = basicEffect_.getGraphicsDeviceInternal();
            device.SetVertexBuffer(&vertexBuffer_);
            device.setIndicesProperty(&indexBuffer_);

            for (auto& pass : basicEffect_.getCurrentTechniqueProperty()->getPassesProperty()) {
                pass.Apply();
                device.DrawIndexedPrimitives(
                    PrimitiveType::LineList, 0, 0, VertexCount, 0, IndexCount / 2);
            }

            device.SetVertexBuffer(nullptr);
            device.setIndicesProperty(nullptr);
        }
        IndexCount  = 0;
        VertexCount = 0;
    }

    bool Reserve(int numVerts, int numIndices) {
        if (numVerts > MAX_VERTS || numIndices > MAX_INDICES)
            return false;
        if (VertexCount + numVerts > MAX_VERTS || IndexCount + numIndices >= MAX_INDICES)
            FlushDrawing();
        return true;
    }
};

} // namespace CollisionSample
