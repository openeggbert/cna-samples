#pragma once

#include <cmath>
#include <stdexcept>

#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "GeometricPrimitive.hpp"

namespace Primitives3D
{
    class CylinderPrimitive : public GeometricPrimitive
    {
        static Vector3 GetCircleVector(int i, int tessellation)
        {
            float angle = i * MathHelper::TwoPi / tessellation;
            return Vector3(std::cos(angle), 0.0f, std::sin(angle));
        }

        void CreateCap(int tessellation, float height, float radius, Vector3 normal)
        {
            for (int i = 0; i < tessellation - 2; ++i)
            {
                if (normal.Y > 0)
                {
                    AddIndex(CurrentVertex());
                    AddIndex(CurrentVertex() + (i + 1) % tessellation);
                    AddIndex(CurrentVertex() + (i + 2) % tessellation);
                }
                else
                {
                    AddIndex(CurrentVertex());
                    AddIndex(CurrentVertex() + (i + 2) % tessellation);
                    AddIndex(CurrentVertex() + (i + 1) % tessellation);
                }
            }
            for (int i = 0; i < tessellation; ++i)
            {
                Vector3 pos = GetCircleVector(i, tessellation) * radius + normal * height;
                AddVertex(pos, normal);
            }
        }

    public:
        explicit CylinderPrimitive(GraphicsDevice& device, float height = 1.0f, float diameter = 1.0f, int tessellation = 32)
        {
            if (tessellation < 3)
                throw std::invalid_argument("tessellation");

            height /= 2.0f;
            float radius = diameter / 2.0f;

            for (int i = 0; i < tessellation; ++i)
            {
                Vector3 normal = GetCircleVector(i, tessellation);
                AddVertex(normal * radius + Vector3::Up   * height, normal);
                AddVertex(normal * radius + Vector3::Down * height, normal);

                AddIndex(i * 2);
                AddIndex(i * 2 + 1);
                AddIndex((i * 2 + 2) % (tessellation * 2));

                AddIndex(i * 2 + 1);
                AddIndex((i * 2 + 3) % (tessellation * 2));
                AddIndex((i * 2 + 2) % (tessellation * 2));
            }

            CreateCap(tessellation, height, radius, Vector3::Up);
            CreateCap(tessellation, height, radius, Vector3::Down);

            InitializePrimitive(device);
        }
    };
}
