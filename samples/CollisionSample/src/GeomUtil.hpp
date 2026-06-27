#pragma once
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace CollisionSample {

using namespace Microsoft::Xna::Framework;

struct GeomUtil {
    static void PerspectiveTransform(const Vector3& position, const Matrix& matrix, Vector3& result) {
        float w = position.X * matrix.M14 + position.Y * matrix.M24 + position.Z * matrix.M34 + matrix.M44;
        float winv = 1.0f / w;
        float x = position.X * matrix.M11 + position.Y * matrix.M21 + position.Z * matrix.M31 + matrix.M41;
        float y = position.X * matrix.M12 + position.Y * matrix.M22 + position.Z * matrix.M32 + matrix.M42;
        float z = position.X * matrix.M13 + position.Y * matrix.M23 + position.Z * matrix.M33 + matrix.M43;
        result.X = x * winv;
        result.Y = y * winv;
        result.Z = z * winv;
    }
};

} // namespace CollisionSample
