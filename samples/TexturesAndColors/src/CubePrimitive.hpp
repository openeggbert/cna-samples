#pragma once

#include "GeometricPrimitive.hpp"

namespace Primitives3D
{
    class CubePrimitive : public GeometricPrimitive
    {
    public:
        explicit CubePrimitive(GraphicsDevice& device, float size = 1.0f)
        {
            Vector3 normals[] = {
                Vector3(0, 0, 1), Vector3(0, 0,-1),
                Vector3(1, 0, 0), Vector3(-1, 0, 0),
                Vector3(0, 1, 0), Vector3(0,-1, 0),
            };

            for (auto& normal : normals)
            {
                Vector3 side1(normal.Y, normal.Z, normal.X);
                Vector3 side2 = Vector3::Cross(normal, side1);

                AddIndex(CurrentVertex() + 0);
                AddIndex(CurrentVertex() + 1);
                AddIndex(CurrentVertex() + 2);
                AddIndex(CurrentVertex() + 0);
                AddIndex(CurrentVertex() + 2);
                AddIndex(CurrentVertex() + 3);

                AddVertex((normal - side1 - side2) * (size / 2.0f), normal);
                AddVertex((normal - side1 + side2) * (size / 2.0f), normal);
                AddVertex((normal + side1 + side2) * (size / 2.0f), normal);
                AddVertex((normal + side1 - side2) * (size / 2.0f), normal);
            }

            InitializePrimitive(device);
        }
    };
}
