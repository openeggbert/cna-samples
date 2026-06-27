#pragma once
#include <cmath>
#include "GeometricPrimitive.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"

namespace Bounce {

class SpherePrimitive : public GeometricPrimitive {
public:
    explicit SpherePrimitive(GraphicsDevice& graphicsDevice)
        : SpherePrimitive(graphicsDevice, 1.0f, 8) {}

    SpherePrimitive(GraphicsDevice& graphicsDevice, float diameter, int tessellation) {
        int verticalSegments   = tessellation;
        int horizontalSegments = tessellation * 2;
        float radius = diameter / 2.0f;

        AddVertex(Vector3::Down * radius, Vector3::Down);

        for (int i = 0; i < verticalSegments - 1; i++) {
            float latitude = ((i + 1) * MathHelper::Pi / verticalSegments) - MathHelper::PiOver2;
            float dy  = std::sin(latitude);
            float dxz = std::cos(latitude);

            for (int j = 0; j < horizontalSegments; j++) {
                float longitude = j * MathHelper::TwoPi / horizontalSegments;
                float dx = std::cos(longitude) * dxz;
                float dz = std::sin(longitude) * dxz;
                Vector3 normal(dx, dy, dz);
                AddVertex(normal * radius, normal);
            }
        }

        AddVertex(Vector3::Up * radius, Vector3::Up);

        for (int i = 0; i < horizontalSegments; i++) {
            AddIndex(0);
            AddIndex(1 + (i + 1) % horizontalSegments);
            AddIndex(1 + i);
        }

        for (int i = 0; i < verticalSegments - 2; i++) {
            for (int j = 0; j < horizontalSegments; j++) {
                int nextI = i + 1;
                int nextJ = (j + 1) % horizontalSegments;
                AddIndex(1 + i     * horizontalSegments + j);
                AddIndex(1 + i     * horizontalSegments + nextJ);
                AddIndex(1 + nextI * horizontalSegments + j);
                AddIndex(1 + i     * horizontalSegments + nextJ);
                AddIndex(1 + nextI * horizontalSegments + nextJ);
                AddIndex(1 + nextI * horizontalSegments + j);
            }
        }

        for (int i = 0; i < horizontalSegments; i++) {
            AddIndex(CurrentVertex() - 1);
            AddIndex(CurrentVertex() - 2 - (i + 1) % horizontalSegments);
            AddIndex(CurrentVertex() - 2 - i);
        }

        InitializePrimitive(graphicsDevice);
    }
};

} // namespace Bounce
