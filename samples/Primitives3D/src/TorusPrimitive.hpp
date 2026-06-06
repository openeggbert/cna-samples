#pragma once

#include <cmath>
#include <stdexcept>

#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "GeometricPrimitive.hpp"

namespace Primitives3D
{
    class TorusPrimitive : public GeometricPrimitive
    {
    public:
        explicit TorusPrimitive(GraphicsDevice& device, float diameter = 1.0f, float thickness = 0.333f, int tessellation = 32)
        {
            if (tessellation < 3)
                throw std::invalid_argument("tessellation");

            for (int i = 0; i < tessellation; ++i)
            {
                float outerAngle = i * MathHelper::TwoPi / tessellation;

                Matrix transform = Matrix::CreateTranslation(diameter / 2.0f, 0.0f, 0.0f) *
                                   Matrix::CreateRotationY(outerAngle);

                for (int j = 0; j < tessellation; ++j)
                {
                    float innerAngle = j * MathHelper::TwoPi / tessellation;
                    float dx = std::cos(innerAngle);
                    float dy = std::sin(innerAngle);

                    Vector3 normal(dx, dy, 0.0f);
                    Vector3 position = normal * (thickness / 2.0f);

                    position = Vector3::Transform(position, transform);
                    normal   = Vector3::TransformNormal(normal, transform);

                    AddVertex(position, normal);

                    int nextI = (i + 1) % tessellation;
                    int nextJ = (j + 1) % tessellation;

                    AddIndex(i     * tessellation + j);
                    AddIndex(i     * tessellation + nextJ);
                    AddIndex(nextI * tessellation + j);

                    AddIndex(i     * tessellation + nextJ);
                    AddIndex(nextI * tessellation + nextJ);
                    AddIndex(nextI * tessellation + j);
                }
            }

            InitializePrimitive(device);
        }
    };
}
