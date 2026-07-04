#pragma once

// SpherePrimitiveTextured.hpp — C++ port of SpherePrimitiveTextured.cs (XNA
// 4.0 SoccerPitch sample). Sphere with a spherical (reflection-map-style) UV
// derived directly from each vertex's normal, matching the original.

#include <cmath>
#include <stdexcept>

#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

#include "ProceduralPrimitive.hpp"

namespace SoccerPitch {

using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Graphics::VertexPositionNormalTexture;

class SpherePrimitiveTextured : public ProceduralPrimitive<VertexPositionNormalTexture> {
public:
    static constexpr int DefaultTessellation = 6;
    static constexpr float DefaultSize = 1.0f;

    explicit SpherePrimitiveTextured(GraphicsDevice& device, float diameter = DefaultSize,
                                      int tessellation = DefaultTessellation) {
        if (tessellation < 3)
            throw std::invalid_argument("tessellation must be greater than 3");

        int verticalSegments = tessellation;
        int horizontalSegments = tessellation * 2;
        float radius = diameter / 2.0f;

        VertexPositionNormalTexture vertex;

        // Start with a single vertex at the bottom of the sphere.
        vertex.Position = Vector3::Down * radius;
        vertex.Normal = Vector3::Down;
        vertex.TextureCoordinate = Vector2(Vector3::Down.X, Vector3::Down.Y);
        AddVertex(vertex);

        // Create rings of vertices at progressively higher latitudes.
        for (int i = 0; i < verticalSegments - 1; ++i) {
            float latitude = ((float)(i + 1) * MathHelper::Pi / (float)verticalSegments) - MathHelper::PiOver2;
            float dy = std::sin(latitude);
            float dxz = std::cos(latitude);

            for (int j = 0; j < horizontalSegments; ++j) {
                float longitude = (float)j * MathHelper::TwoPi / (float)horizontalSegments;
                float dx = std::cos(longitude) * dxz;
                float dz = std::sin(longitude) * dxz;

                Vector3 normal(dx, dy, dz);
                vertex.Position = normal * radius;
                vertex.TextureCoordinate = Vector2(normal.X, normal.Y);
                vertex.Normal = normal;
                AddVertex(vertex);
            }
        }

        // Finish with a single vertex at the top of the sphere.
        vertex.Position = Vector3::Up * radius;
        vertex.Normal = Vector3::Up;
        vertex.TextureCoordinate = Vector2(Vector3::Up.X, Vector3::Up.Y);
        AddVertex(vertex);

        // Create a fan connecting the bottom vertex to the bottom latitude ring.
        for (int i = 0; i < horizontalSegments; ++i) {
            AddIndex(0);
            AddIndex(1 + (i + 1) % horizontalSegments);
            AddIndex(1 + i);
        }

        // Fill the sphere body with triangles joining each pair of latitude rings.
        for (int i = 0; i < verticalSegments - 2; ++i) {
            for (int j = 0; j < horizontalSegments; ++j) {
                int nextI = i + 1;
                int nextJ = (j + 1) % horizontalSegments;

                AddIndex(1 + i * horizontalSegments + j);
                AddIndex(1 + i * horizontalSegments + nextJ);
                AddIndex(1 + nextI * horizontalSegments + j);

                AddIndex(1 + i * horizontalSegments + nextJ);
                AddIndex(1 + nextI * horizontalSegments + nextJ);
                AddIndex(1 + nextI * horizontalSegments + j);
            }
        }

        // Create a fan connecting the top vertex to the top latitude ring.
        for (int i = 0; i < horizontalSegments; ++i) {
            AddIndex(CurrentVertex() - 1);
            AddIndex(CurrentVertex() - 2 - (i + 1) % horizontalSegments);
            AddIndex(CurrentVertex() - 2 - i);
        }

        InitializePrimitive(device);
    }
};

} // namespace SoccerPitch
