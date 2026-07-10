#pragma once

// Port of DebugDraw.cs (XNA 4.0 TiltPerspective sample) -- builds and draws a
// single textured mesh (here, the inside of the play box) out of
// VertexPositionNormalTexture data. Unlike GeometricPrimitive.hpp/
// SpherePrimitive.hpp, this file needed NO vertex-type workaround: the C#
// original already used the real, built-in VertexPositionNormalTexture type
// (it has real UVs, unlike the sphere primitives), which CNA already
// supports end-to-end (DEFERRED.md item #5).

#include <vector>

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

namespace TiltPerspectiveSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class DebugDraw {
public:
    DebugDraw(GraphicsDevice& device, const std::vector<VertexPositionNormalTexture>& vertices,
              const std::vector<std::uint16_t>& indices) {
        primitiveCount_ = static_cast<int>(indices.size()) / 3;
        indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices.size()));
        indexBuffer_->SetData(indices.data(), 0, static_cast<int>(indices.size()));

        vertexCount_ = static_cast<int>(vertices.size());
        vertexBuffer_ = std::make_unique<VertexBuffer>(device, static_cast<int>(vertices.size()));
        vertexBuffer_->SetData(vertices.data(), 0, static_cast<int>(vertices.size()));

        basicEffect_ = std::make_unique<BasicEffect>(device);
    }

    // Matches `DebugDraw.CreateBoxInterior()` -- builds a box with all 5
    // reachable faces oriented inward (the near/screen-side face, Z ==
    // box.Max.Z, is intentionally never emitted -- matching
    // BallSimulation.AddWalls()'s own commented-out near-wall, so the balls
    // are visible popping toward the viewer with nothing drawn in front of
    // the camera).
    static DebugDraw CreateBoxInterior(GraphicsDevice& device, const BoundingBox& box) {
        Vector3 size = box.Max - box.Min;
        std::vector<VertexPositionNormalTexture> vertices;
        std::vector<std::uint16_t> indices;

        AppendQuad(Vector3(box.Min.X, box.Min.Y, box.Min.Z), Vector3(size.X, 0, 0), Vector3(0, size.Y, 0), vertices,
                   indices);

        AppendQuad(Vector3(box.Min.X, box.Min.Y, box.Min.Z), Vector3(0, size.Y, 0), Vector3(0, 0, size.Z), vertices,
                   indices);

        AppendQuad(Vector3(box.Min.X, box.Min.Y, box.Min.Z), Vector3(0, 0, size.Z), Vector3(size.X, 0, 0), vertices,
                   indices);

        AppendQuad(Vector3(box.Max.X, box.Max.Y, box.Min.Z), Vector3(0, -size.Y, 0), Vector3(0, 0, size.Z), vertices,
                   indices);

        AppendQuad(Vector3(box.Max.X, box.Max.Y, box.Min.Z), Vector3(0, 0, size.Z), Vector3(-size.X, 0, 0), vertices,
                   indices);

        return DebugDraw(device, vertices, indices);
    }

    // Matches `Draw(ref Matrix world, ref Matrix view, ref Matrix projection, Texture2D texture)`.
    void Draw(const Matrix& world, const Matrix& view, const Matrix& projection, Texture2D& texture) {
        basicEffect_->World = world;
        basicEffect_->View = view;
        basicEffect_->Projection = projection;

        basicEffect_->setTextureProperty(&texture);
        basicEffect_->setTextureEnabledProperty(true);

        basicEffect_->setLightingEnabledProperty(true);
        basicEffect_->getDirectionalLight0Property().setEnabledProperty(true);
        basicEffect_->getDirectionalLight0Property().setDirectionProperty(-Vector3::UnitZ);
        basicEffect_->setAmbientLightColorProperty(Vector3(0.5f, 0.5f, 0.5f));

        basicEffect_->VertexColorEnabled = false;

        GraphicsDevice& device = basicEffect_->getGraphicsDeviceInternal();
        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());

        for (auto& pass : basicEffect_->getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0, primitiveCount_);
        }

        device.SetVertexBuffer(nullptr);
        device.setIndicesProperty(nullptr);
    }

    // NOXNA: exposed so TiltPerspectiveGame can set DirectionalLight0.Direction
    // every frame (accelerometer-driven lighting), matching the original's
    // `worldGeometry.BasicEffect.DirectionalLight0.Direction = lightDirection;`
    // direct field access.
    [[nodiscard]] BasicEffect& getBasicEffectProperty() { return *basicEffect_; }

private:
    static void AppendQuad(const Vector3& origin, const Vector3& dx, const Vector3& dy,
                           std::vector<VertexPositionNormalTexture>& vertices, std::vector<std::uint16_t>& indices) {
        Vector3 norm = Vector3::Cross(dx, dy);
        norm.Normalize();
        int i = static_cast<int>(vertices.size());

        vertices.emplace_back(origin, norm, Vector2(0, 0));
        vertices.emplace_back(origin + dx, norm, Vector2(1, 0));
        vertices.emplace_back(origin + dy, norm, Vector2(0, 1));
        vertices.emplace_back(origin + dx + dy, norm, Vector2(1, 1));

        indices.push_back(static_cast<std::uint16_t>(i + 0));
        indices.push_back(static_cast<std::uint16_t>(i + 2));
        indices.push_back(static_cast<std::uint16_t>(i + 1));
        indices.push_back(static_cast<std::uint16_t>(i + 1));
        indices.push_back(static_cast<std::uint16_t>(i + 2));
        indices.push_back(static_cast<std::uint16_t>(i + 3));
    }

    int vertexCount_ = 0;
    int primitiveCount_ = 0;
    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;
    std::unique_ptr<BasicEffect> basicEffect_;
};

} // namespace TiltPerspectiveSample
