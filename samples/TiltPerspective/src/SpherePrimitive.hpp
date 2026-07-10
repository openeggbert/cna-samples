#pragma once

// Port of SpherePrimitive.cs (XNA 4.0 TiltPerspective sample) -- procedurally
// generates a UV sphere's vertices/indices via GeometricPrimitive's
// AddVertex()/AddIndex() helpers. Direct, literal port of the original's
// math; see GeometricPrimitive.hpp for the one adaptation (dummy texture
// coordinates, per DEFERRED.md item #5).

#include <cmath>
#include <stdexcept>

#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "GeometricPrimitive.hpp"

namespace TiltPerspectiveSample {

class SpherePrimitive : public GeometricPrimitive {
public:
    // Matches `SpherePrimitive(GraphicsDevice)` -- default diameter 1, tessellation 8.
    explicit SpherePrimitive(GraphicsDevice& device) : SpherePrimitive(device, 1.0f, 8) {}

    // Matches `SpherePrimitive(GraphicsDevice, float diameter, int tessellation)`.
    SpherePrimitive(GraphicsDevice& device, float diameter, int tessellation) {
        if (tessellation < 3)
            throw std::invalid_argument("tessellation");

        int verticalSegments = tessellation;
        int horizontalSegments = tessellation * 2;

        float radius = diameter / 2.0f;

        // Start with a single vertex at the bottom of the sphere.
        AddVertex(Vector3::Down * radius, Vector3::Down);

        // Create rings of vertices at progressively higher latitudes.
        for (int i = 0; i < verticalSegments - 1; ++i) {
            float latitude = ((i + 1) * MathHelper::Pi / verticalSegments) - MathHelper::PiOver2;

            float dy = std::sin(latitude);
            float dxz = std::cos(latitude);

            // Create a single ring of vertices at this latitude.
            for (int j = 0; j < horizontalSegments; ++j) {
                float longitude = j * MathHelper::TwoPi / horizontalSegments;

                float dx = std::cos(longitude) * dxz;
                float dz = std::sin(longitude) * dxz;

                Vector3 normal(dx, dy, dz);

                AddVertex(normal * radius, normal);
            }
        }

        // Finish with a single vertex at the top of the sphere.
        AddVertex(Vector3::Up * radius, Vector3::Up);

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

} // namespace TiltPerspectiveSample
