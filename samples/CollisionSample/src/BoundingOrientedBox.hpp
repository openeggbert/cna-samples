#pragma once
#include <cmath>
#include <vector>
#include "TriangleTest.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/BoundingFrustum.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Plane.hpp"
#include "Microsoft/Xna/Framework/PlaneIntersectionType.hpp"
#include "Microsoft/Xna/Framework/Quaternion.hpp"
#include "Microsoft/Xna/Framework/Ray.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace CollisionSample {

using namespace Microsoft::Xna::Framework;

struct BoundingOrientedBox {
    static constexpr int CornerCount = 8;
    static constexpr float RAY_EPSILON = 1e-20f;

    Vector3 Center;
    Vector3 HalfExtent;
    Quaternion Orientation;

    BoundingOrientedBox()
        : Center(0,0,0), HalfExtent(0,0,0), Orientation(0,0,0,1) {}

    BoundingOrientedBox(Vector3 center, Vector3 halfExtents, Quaternion orientation)
        : Center(center), HalfExtent(halfExtents), Orientation(orientation) {}

    static BoundingOrientedBox CreateFromBoundingBox(const BoundingBox& box) {
        Vector3 mid = (box.Min + box.Max) * 0.5f;
        Vector3 halfExtent = (box.Max - box.Min) * 0.5f;
        return BoundingOrientedBox(mid, halfExtent, Quaternion::Identity);
    }

    BoundingOrientedBox Transform(Quaternion rotation, Vector3 translation) const {
        return BoundingOrientedBox(
            Vector3::Transform(Center, rotation) + translation,
            HalfExtent,
            Orientation * rotation);
    }

    BoundingOrientedBox Transform(float scale, Quaternion rotation, Vector3 translation) const {
        return BoundingOrientedBox(
            Vector3::Transform(Center * scale, rotation) + translation,
            HalfExtent * scale,
            Orientation * rotation);
    }

    bool Equals(const BoundingOrientedBox& other) const {
        return Center == other.Center && HalfExtent == other.HalfExtent && Orientation == other.Orientation;
    }

    // Test vs. BoundingBox
    bool Intersects(const BoundingBox& box) const {
        Vector3 boxCenter = (box.Max + box.Min) * 0.5f;
        Vector3 boxHalfExtent = (box.Max - box.Min) * 0.5f;
        Matrix mb = Matrix::CreateFromQuaternion(Orientation);
        mb.setTranslationProperty(Center - boxCenter);
        return ContainsRelativeBox(boxHalfExtent, HalfExtent, mb) != ContainmentType::Disjoint;
    }

    ContainmentType Contains(const BoundingBox& box) const {
        Vector3 boxCenter = (box.Max + box.Min) * 0.5f;
        Vector3 boxHalfExtent = (box.Max - box.Min) * 0.5f;
        Quaternion relOrient = Quaternion::Conjugate(Orientation);
        Matrix relTransform = Matrix::CreateFromQuaternion(relOrient);
        relTransform.setTranslationProperty(Vector3::TransformNormal(boxCenter - Center, relTransform));
        return ContainsRelativeBox(HalfExtent, boxHalfExtent, relTransform);
    }

    static ContainmentType Contains(const BoundingBox& boxA, const BoundingOrientedBox& oboxB) {
        Vector3 boxA_halfExtent = (boxA.Max - boxA.Min) * 0.5f;
        Vector3 boxA_center = (boxA.Max + boxA.Min) * 0.5f;
        Matrix mb = Matrix::CreateFromQuaternion(oboxB.Orientation);
        mb.setTranslationProperty(oboxB.Center - boxA_center);
        return ContainsRelativeBox(boxA_halfExtent, oboxB.HalfExtent, mb);
    }

    // Test vs. BoundingOrientedBox
    bool Intersects(const BoundingOrientedBox& other) const {
        return Contains(other) != ContainmentType::Disjoint;
    }

    ContainmentType Contains(const BoundingOrientedBox& other) const {
        Quaternion invOrient = Quaternion::Conjugate(Orientation);
        Quaternion relOrient = Quaternion::Multiply(invOrient, other.Orientation);
        Matrix relTransform = Matrix::CreateFromQuaternion(relOrient);
        relTransform.setTranslationProperty(Vector3::Transform(other.Center - Center, invOrient));
        return ContainsRelativeBox(HalfExtent, other.HalfExtent, relTransform);
    }

    // Test vs. BoundingFrustum
    ContainmentType Contains(const BoundingFrustum& frustum) const {
        BoundingFrustum temp = ConvertToFrustum();
        return temp.Contains(frustum);
    }

    bool Intersects(const BoundingFrustum& frustum) const {
        return Contains(frustum) != ContainmentType::Disjoint;
    }

    static ContainmentType Contains(const BoundingFrustum& frustum, const BoundingOrientedBox& obox) {
        return frustum.Contains(obox.ConvertToFrustum());
    }

    // Test vs. BoundingSphere
    ContainmentType Contains(const BoundingSphere& sphere) const {
        Quaternion iq = Quaternion::Conjugate(Orientation);
        Vector3 localCenter = Vector3::Transform(sphere.Center - Center, iq);
        float dx = std::abs(localCenter.X) - HalfExtent.X;
        float dy = std::abs(localCenter.Y) - HalfExtent.Y;
        float dz = std::abs(localCenter.Z) - HalfExtent.Z;
        float r = sphere.Radius;
        if (dx <= -r && dy <= -r && dz <= -r)
            return ContainmentType::Contains;
        dx = std::max(dx, 0.0f);
        dy = std::max(dy, 0.0f);
        dz = std::max(dz, 0.0f);
        if (dx*dx + dy*dy + dz*dz >= r*r)
            return ContainmentType::Disjoint;
        return ContainmentType::Intersects;
    }

    bool Intersects(const BoundingSphere& sphere) const {
        Quaternion iq = Quaternion::Conjugate(Orientation);
        Vector3 localCenter = Vector3::Transform(sphere.Center - Center, iq);
        float dx = std::abs(localCenter.X) - HalfExtent.X;
        float dy = std::abs(localCenter.Y) - HalfExtent.Y;
        float dz = std::abs(localCenter.Z) - HalfExtent.Z;
        dx = std::max(dx, 0.0f);
        dy = std::max(dy, 0.0f);
        dz = std::max(dz, 0.0f);
        float r = sphere.Radius;
        return dx*dx + dy*dy + dz*dz < r*r;
    }

    static ContainmentType Contains(const BoundingSphere& sphere, const BoundingOrientedBox& box) {
        Quaternion iq = Quaternion::Conjugate(box.Orientation);
        Vector3 localCenter = Vector3::Transform(sphere.Center - box.Center, iq);
        localCenter.X = std::abs(localCenter.X);
        localCenter.Y = std::abs(localCenter.Y);
        localCenter.Z = std::abs(localCenter.Z);
        float rSquared = sphere.Radius * sphere.Radius;
        if ((localCenter + box.HalfExtent).LengthSquared() <= rSquared)
            return ContainmentType::Contains;
        Vector3 d = localCenter - box.HalfExtent;
        d.X = std::max(d.X, 0.0f);
        d.Y = std::max(d.Y, 0.0f);
        d.Z = std::max(d.Z, 0.0f);
        if (d.LengthSquared() >= rSquared)
            return ContainmentType::Disjoint;
        return ContainmentType::Intersects;
    }

    // Test vs. point/ray/plane
    bool Contains(const Vector3& point) const {
        Quaternion qinv = Quaternion::Conjugate(Orientation);
        Vector3 plocal = Vector3::Transform(point - Center, qinv);
        return std::abs(plocal.X) <= HalfExtent.X &&
               std::abs(plocal.Y) <= HalfExtent.Y &&
               std::abs(plocal.Z) <= HalfExtent.Z;
    }

    std::optional<float> Intersects(const Ray& ray) const {
        Matrix R = Matrix::CreateFromQuaternion(Orientation);
        Vector3 TOrigin = Center - ray.Position;
        float t_min = -std::numeric_limits<float>::max();
        float t_max = std::numeric_limits<float>::max();

        // X axis
        float axisDotOrigin = Vector3::Dot(R.getRightProperty(), TOrigin);
        float axisDotDir    = Vector3::Dot(R.getRightProperty(), ray.Direction);
        if (axisDotDir >= -RAY_EPSILON && axisDotDir <= RAY_EPSILON) {
            if ((-axisDotOrigin - HalfExtent.X) > 0.0f || (-axisDotOrigin + HalfExtent.X) > 0.0f)
                return std::nullopt;
        } else {
            float t1 = (axisDotOrigin - HalfExtent.X) / axisDotDir;
            float t2 = (axisDotOrigin + HalfExtent.X) / axisDotDir;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > t_min) t_min = t1;
            if (t2 < t_max) t_max = t2;
            if (t_max < 0.0f || t_min > t_max) return std::nullopt;
        }

        // Y axis
        axisDotOrigin = Vector3::Dot(R.getUpProperty(), TOrigin);
        axisDotDir    = Vector3::Dot(R.getUpProperty(), ray.Direction);
        if (axisDotDir >= -RAY_EPSILON && axisDotDir <= RAY_EPSILON) {
            if ((-axisDotOrigin - HalfExtent.Y) > 0.0f || (-axisDotOrigin + HalfExtent.Y) > 0.0f)
                return std::nullopt;
        } else {
            float t1 = (axisDotOrigin - HalfExtent.Y) / axisDotDir;
            float t2 = (axisDotOrigin + HalfExtent.Y) / axisDotDir;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > t_min) t_min = t1;
            if (t2 < t_max) t_max = t2;
            if (t_max < 0.0f || t_min > t_max) return std::nullopt;
        }

        // Z axis (Forward in XNA = -Z, Backward = +Z)
        axisDotOrigin = Vector3::Dot(R.getForwardProperty(), TOrigin);
        axisDotDir    = Vector3::Dot(R.getForwardProperty(), ray.Direction);
        if (axisDotDir >= -RAY_EPSILON && axisDotDir <= RAY_EPSILON) {
            if ((-axisDotOrigin - HalfExtent.Z) > 0.0f || (-axisDotOrigin + HalfExtent.Z) > 0.0f)
                return std::nullopt;
        } else {
            float t1 = (axisDotOrigin - HalfExtent.Z) / axisDotDir;
            float t2 = (axisDotOrigin + HalfExtent.Z) / axisDotDir;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > t_min) t_min = t1;
            if (t2 < t_max) t_max = t2;
            if (t_max < 0.0f || t_min > t_max) return std::nullopt;
        }

        return t_min;
    }

    PlaneIntersectionType Intersects(const Plane& plane) const {
        float dist = plane.DotCoordinate(Center);
        Vector3 localNormal = Vector3::Transform(plane.Normal, Quaternion::Conjugate(Orientation));
        float r = std::abs(HalfExtent.X * localNormal.X)
                + std::abs(HalfExtent.Y * localNormal.Y)
                + std::abs(HalfExtent.Z * localNormal.Z);
        if (dist > r) return PlaneIntersectionType::Front;
        if (dist < -r) return PlaneIntersectionType::Back;
        return PlaneIntersectionType::Intersecting;
    }

    // GetCorners
    std::vector<Vector3> GetCorners() const {
        std::vector<Vector3> corners(CornerCount);
        Matrix m = Matrix::CreateFromQuaternion(Orientation);
        Vector3 hX = m.getLeftProperty()     * HalfExtent.X;
        Vector3 hY = m.getUpProperty()       * HalfExtent.Y;
        Vector3 hZ = m.getBackwardProperty() * HalfExtent.Z;
        corners[0] = Center - hX + hY + hZ;
        corners[1] = Center + hX + hY + hZ;
        corners[2] = Center + hX - hY + hZ;
        corners[3] = Center - hX - hY + hZ;
        corners[4] = Center - hX + hY - hZ;
        corners[5] = Center + hX + hY - hZ;
        corners[6] = Center + hX - hY - hZ;
        corners[7] = Center - hX - hY - hZ;
        return corners;
    }

    // Static box-vs-box relative containment test
    static ContainmentType ContainsRelativeBox(const Vector3& hA, const Vector3& hB, const Matrix& mB) {
        Vector3 mB_T = mB.getTranslationProperty();
        Vector3 mB_TA(std::abs(mB_T.X), std::abs(mB_T.Y), std::abs(mB_T.Z));

        Vector3 bX  = mB.getRightProperty();
        Vector3 bY  = mB.getUpProperty();
        Vector3 bZ  = mB.getBackwardProperty();
        Vector3 hx_B = bX * hB.X;
        Vector3 hy_B = bY * hB.Y;
        Vector3 hz_B = bZ * hB.Z;

        // Containment check
        float projx_B = std::abs(hx_B.X) + std::abs(hy_B.X) + std::abs(hz_B.X);
        float projy_B = std::abs(hx_B.Y) + std::abs(hy_B.Y) + std::abs(hz_B.Y);
        float projz_B = std::abs(hx_B.Z) + std::abs(hy_B.Z) + std::abs(hz_B.Z);
        if (mB_TA.X + projx_B <= hA.X && mB_TA.Y + projy_B <= hA.Y && mB_TA.Z + projz_B <= hA.Z)
            return ContainmentType::Contains;

        // Separation along A's axes
        if (mB_TA.X >= hA.X + std::abs(hx_B.X) + std::abs(hy_B.X) + std::abs(hz_B.X)) return ContainmentType::Disjoint;
        if (mB_TA.Y >= hA.Y + std::abs(hx_B.Y) + std::abs(hy_B.Y) + std::abs(hz_B.Y)) return ContainmentType::Disjoint;
        if (mB_TA.Z >= hA.Z + std::abs(hx_B.Z) + std::abs(hy_B.Z) + std::abs(hz_B.Z)) return ContainmentType::Disjoint;

        // Separation along B's axes
        if (std::abs(Vector3::Dot(mB_T, bX)) >= std::abs(hA.X*bX.X)+std::abs(hA.Y*bX.Y)+std::abs(hA.Z*bX.Z)+hB.X) return ContainmentType::Disjoint;
        if (std::abs(Vector3::Dot(mB_T, bY)) >= std::abs(hA.X*bY.X)+std::abs(hA.Y*bY.Y)+std::abs(hA.Z*bY.Z)+hB.Y) return ContainmentType::Disjoint;
        if (std::abs(Vector3::Dot(mB_T, bZ)) >= std::abs(hA.X*bZ.X)+std::abs(hA.Y*bZ.Y)+std::abs(hA.Z*bZ.Z)+hB.Z) return ContainmentType::Disjoint;

        // Cross-product axes
        Vector3 axis;

        axis = Vector3(0, -bX.Z, bX.Y);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.Y*axis.Y)+std::abs(hA.Z*axis.Z)+std::abs(Vector3::Dot(axis,hy_B))+std::abs(Vector3::Dot(axis,hz_B))) return ContainmentType::Disjoint;

        axis = Vector3(0, -bY.Z, bY.Y);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.Y*axis.Y)+std::abs(hA.Z*axis.Z)+std::abs(Vector3::Dot(axis,hz_B))+std::abs(Vector3::Dot(axis,hx_B))) return ContainmentType::Disjoint;

        axis = Vector3(0, -bZ.Z, bZ.Y);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.Y*axis.Y)+std::abs(hA.Z*axis.Z)+std::abs(Vector3::Dot(axis,hx_B))+std::abs(Vector3::Dot(axis,hy_B))) return ContainmentType::Disjoint;

        axis = Vector3(bX.Z, 0, -bX.X);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.Z*axis.Z)+std::abs(hA.X*axis.X)+std::abs(Vector3::Dot(axis,hy_B))+std::abs(Vector3::Dot(axis,hz_B))) return ContainmentType::Disjoint;

        axis = Vector3(bY.Z, 0, -bY.X);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.Z*axis.Z)+std::abs(hA.X*axis.X)+std::abs(Vector3::Dot(axis,hz_B))+std::abs(Vector3::Dot(axis,hx_B))) return ContainmentType::Disjoint;

        axis = Vector3(bZ.Z, 0, -bZ.X);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.Z*axis.Z)+std::abs(hA.X*axis.X)+std::abs(Vector3::Dot(axis,hx_B))+std::abs(Vector3::Dot(axis,hy_B))) return ContainmentType::Disjoint;

        axis = Vector3(-bX.Y, bX.X, 0);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.X*axis.X)+std::abs(hA.Y*axis.Y)+std::abs(Vector3::Dot(axis,hy_B))+std::abs(Vector3::Dot(axis,hz_B))) return ContainmentType::Disjoint;

        axis = Vector3(-bY.Y, bY.X, 0);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.X*axis.X)+std::abs(hA.Y*axis.Y)+std::abs(Vector3::Dot(axis,hz_B))+std::abs(Vector3::Dot(axis,hx_B))) return ContainmentType::Disjoint;

        axis = Vector3(-bZ.Y, bZ.X, 0);
        if (std::abs(Vector3::Dot(mB_T,axis)) >= std::abs(hA.X*axis.X)+std::abs(hA.Y*axis.Y)+std::abs(Vector3::Dot(axis,hx_B))+std::abs(Vector3::Dot(axis,hy_B))) return ContainmentType::Disjoint;

        return ContainmentType::Intersects;
    }

    BoundingFrustum ConvertToFrustum() const {
        Quaternion invOrientation = Quaternion::Conjugate(Orientation);
        float sx = 1.0f / HalfExtent.X;
        float sy = 1.0f / HalfExtent.Y;
        float sz = 0.5f / HalfExtent.Z;
        Matrix temp = Matrix::CreateFromQuaternion(invOrientation);
        temp.M11 *= sx; temp.M21 *= sx; temp.M31 *= sx;
        temp.M12 *= sy; temp.M22 *= sy; temp.M32 *= sy;
        temp.M13 *= sz; temp.M23 *= sz; temp.M33 *= sz;
        temp.setTranslationProperty(Vector3::UnitZ * 0.5f + Vector3::TransformNormal(-Center, temp));
        return BoundingFrustum(temp);
    }

    bool operator==(const BoundingOrientedBox& b) const { return Equals(b); }
    bool operator!=(const BoundingOrientedBox& b) const { return !Equals(b); }
};

// TriangleTest methods that need BoundingOrientedBox (defined here to avoid circular deps)
inline bool TriangleTest_IntersectsOBox(const BoundingOrientedBox& obox, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
    Quaternion qinv = Quaternion::Conjugate(obox.Orientation);
    Matrix minv = Matrix::CreateFromQuaternion(qinv);
    Triangle localTri;
    localTri.V0 = Vector3::TransformNormal(v0 - obox.Center, minv);
    localTri.V1 = Vector3::TransformNormal(v1 - obox.Center, minv);
    localTri.V2 = Vector3::TransformNormal(v2 - obox.Center, minv);
    return TriangleTest::OriginBoxContains(obox.HalfExtent, localTri) != ContainmentType::Disjoint;
}

inline ContainmentType TriangleTest_ContainsOBox(const BoundingOrientedBox& obox, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
    Quaternion qinv = Quaternion::Conjugate(obox.Orientation);
    Matrix minv = Matrix::CreateFromQuaternion(qinv);
    Triangle localTri;
    localTri.V0 = Vector3::TransformNormal(v0 - obox.Center, minv);
    localTri.V1 = Vector3::TransformNormal(v1 - obox.Center, minv);
    localTri.V2 = Vector3::TransformNormal(v2 - obox.Center, minv);
    return TriangleTest::OriginBoxContains(obox.HalfExtent, localTri);
}

inline ContainmentType TriangleTest_ContainsOBox(const BoundingOrientedBox& obox, const Triangle& triangle) {
    return TriangleTest_ContainsOBox(obox, triangle.V0, triangle.V1, triangle.V2);
}

} // namespace CollisionSample
