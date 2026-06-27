#pragma once
#include <cmath>
#include <optional>
#include "GeomUtil.hpp"
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

struct Triangle {
    Vector3 V0;
    Vector3 V1;
    Vector3 V2;

    Triangle() : V0(0,0,0), V1(0,0,0), V2(0,0,0) {}
    Triangle(Vector3 v0, Vector3 v1, Vector3 v2) : V0(v0), V1(v1), V2(v2) {}
};

struct TriangleTest {
    static constexpr float EPSILON = 1e-20f;

    // Triangle-BoundingBox
    static bool Intersects(const BoundingBox& box, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        Vector3 boxCenter = (box.Max + box.Min) * 0.5f;
        Vector3 boxHalfExtent = (box.Max - box.Min) * 0.5f;
        Triangle localTri;
        localTri.V0 = v0 - boxCenter;
        localTri.V1 = v1 - boxCenter;
        localTri.V2 = v2 - boxCenter;
        return OriginBoxContains(boxHalfExtent, localTri) != ContainmentType::Disjoint;
    }

    static ContainmentType Contains(const BoundingBox& box, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        Vector3 boxCenter = (box.Max + box.Min) * 0.5f;
        Vector3 boxHalfExtent = (box.Max - box.Min) * 0.5f;
        Triangle localTri;
        localTri.V0 = v0 - boxCenter;
        localTri.V1 = v1 - boxCenter;
        localTri.V2 = v2 - boxCenter;
        return OriginBoxContains(boxHalfExtent, localTri);
    }

    static ContainmentType Contains(const BoundingBox& box, const Triangle& triangle) {
        return Contains(box, triangle.V0, triangle.V1, triangle.V2);
    }

    // Triangle-BoundingOrientedBox (forward-declared below; defined in BoundingOrientedBox.hpp)
    // The OBox versions are defined in BoundingOrientedBox.hpp to avoid circular inclusion.

    // Triangle-Sphere
    static bool Intersects(const BoundingSphere& sphere, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        Vector3 p = NearestPointOnTriangle(sphere.Center, v0, v1, v2);
        return Vector3::DistanceSquared(sphere.Center, p) < sphere.Radius * sphere.Radius;
    }

    static bool Intersects(const BoundingSphere& sphere, const Triangle& t) {
        Vector3 p = NearestPointOnTriangle(sphere.Center, t.V0, t.V1, t.V2);
        return Vector3::DistanceSquared(sphere.Center, p) < sphere.Radius * sphere.Radius;
    }

    static ContainmentType Contains(const BoundingSphere& sphere, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        float r2 = sphere.Radius * sphere.Radius;
        if (Vector3::DistanceSquared(v0, sphere.Center) <= r2 &&
            Vector3::DistanceSquared(v1, sphere.Center) <= r2 &&
            Vector3::DistanceSquared(v2, sphere.Center) <= r2)
            return ContainmentType::Contains;
        return Intersects(sphere, v0, v1, v2) ? ContainmentType::Intersects : ContainmentType::Disjoint;
    }

    static ContainmentType Contains(const BoundingSphere& sphere, const Triangle& triangle) {
        return Contains(sphere, triangle.V0, triangle.V1, triangle.V2);
    }

    // Triangle-Frustum
    static bool Intersects(const BoundingFrustum& frustum, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        Matrix m = frustum.getMatrixProperty();
        Triangle localTri;
        GeomUtil::PerspectiveTransform(v0, m, localTri.V0);
        GeomUtil::PerspectiveTransform(v1, m, localTri.V1);
        GeomUtil::PerspectiveTransform(v2, m, localTri.V2);
        BoundingBox box;
        box.Min = Vector3(-1.0f, -1.0f, 0.0f);
        box.Max = Vector3(1.0f, 1.0f, 1.0f);
        return Intersects(box, localTri.V0, localTri.V1, localTri.V2);
    }

    static ContainmentType Contains(const BoundingFrustum& frustum, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        Matrix m = frustum.getMatrixProperty();
        Triangle localTri;
        GeomUtil::PerspectiveTransform(v0, m, localTri.V0);
        GeomUtil::PerspectiveTransform(v1, m, localTri.V1);
        GeomUtil::PerspectiveTransform(v2, m, localTri.V2);
        Vector3 halfExtent(1.0f, 1.0f, 0.5f);
        localTri.V0.Z -= 0.5f;
        localTri.V1.Z -= 0.5f;
        localTri.V2.Z -= 0.5f;
        return OriginBoxContains(halfExtent, localTri);
    }

    static ContainmentType Contains(const BoundingFrustum& frustum, const Triangle& triangle) {
        return Contains(frustum, triangle.V0, triangle.V1, triangle.V2);
    }

    // Triangle-Plane
    static PlaneIntersectionType Intersects(const Plane& plane, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        float dV0 = plane.DotCoordinate(v0);
        float dV1 = plane.DotCoordinate(v1);
        float dV2 = plane.DotCoordinate(v2);
        if (std::min(dV0, std::min(dV1, dV2)) >= 0)
            return PlaneIntersectionType::Front;
        if (std::max(dV0, std::max(dV1, dV2)) <= 0)
            return PlaneIntersectionType::Back;
        return PlaneIntersectionType::Intersecting;
    }

    // Triangle-Ray
    static std::optional<float> Intersects(const Ray& ray, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        Vector3 e1 = v1 - v0;
        Vector3 e2 = v2 - v0;
        Vector3 p = Vector3::Cross(ray.Direction, e2);
        float det = Vector3::Dot(e1, p);
        float t;
        if (det >= EPSILON) {
            Vector3 s = ray.Position - v0;
            float u = Vector3::Dot(s, p);
            if (u < 0 || u > det) return std::nullopt;
            Vector3 q = Vector3::Cross(s, e1);
            float v = Vector3::Dot(ray.Direction, q);
            if (v < 0 || (u + v) > det) return std::nullopt;
            t = Vector3::Dot(e2, q);
            if (t < 0) return std::nullopt;
        } else if (det <= -EPSILON) {
            Vector3 s = ray.Position - v0;
            float u = Vector3::Dot(s, p);
            if (u > 0 || u < det) return std::nullopt;
            Vector3 q = Vector3::Cross(s, e1);
            float v = Vector3::Dot(ray.Direction, q);
            if (v > 0 || (u + v) < det) return std::nullopt;
            t = Vector3::Dot(e2, q);
            if (t > 0) return std::nullopt;
        } else {
            return std::nullopt;
        }
        return t / det;
    }

    static std::optional<float> Intersects(const Ray& ray, const Triangle& tri) {
        return Intersects(ray, tri.V0, tri.V1, tri.V2);
    }

    // Nearest point on triangle to point p
    static Vector3 NearestPointOnTriangle(const Vector3& p, const Vector3& v0, const Vector3& v1, const Vector3& v2) {
        Vector3 D = p - v0;
        Vector3 E1 = v1 - v0;
        Vector3 E2 = v2 - v0;
        float dot11 = E1.LengthSquared();
        float dot12 = Vector3::Dot(E1, E2);
        float dot22 = E2.LengthSquared();
        float dot1d = Vector3::Dot(E1, D);
        float dot2d = Vector3::Dot(E2, D);

        float s = dot1d * dot22 - dot2d * dot12;
        float t = dot2d * dot11 - dot1d * dot12;
        float d = dot11 * dot22 - dot12 * dot12;

        if (dot1d <= 0 && dot2d <= 0)
            return v0;
        if (s <= 0 && dot2d >= 0 && dot2d <= dot22)
            return v0 + E2 * (dot2d / dot22);
        if (t <= 0 && dot1d >= 0 && dot1d <= dot11)
            return v0 + E1 * (dot1d / dot11);
        if (s >= 0 && t >= 0 && s + t <= d) {
            float dr = 1.0f / d;
            return v0 + (s * dr) * E1 + (t * dr) * E2;
        }
        float u12_num = dot2d - dot1d - dot12 + dot11;
        float u12_den = dot22 + dot11 - 2 * dot12;
        if (u12_num <= 0) return v1;
        if (u12_num >= u12_den) return v2;
        return v1 + (v2 - v1) * (u12_num / u12_den);
    }

    // Origin-centered AABB vs triangle (used by box and frustum tests)
    static ContainmentType OriginBoxContains(const Vector3& halfExtent, const Triangle& tri) {
        BoundingBox triBounds;
        triBounds.Min.X = std::min(tri.V0.X, std::min(tri.V1.X, tri.V2.X));
        triBounds.Min.Y = std::min(tri.V0.Y, std::min(tri.V1.Y, tri.V2.Y));
        triBounds.Min.Z = std::min(tri.V0.Z, std::min(tri.V1.Z, tri.V2.Z));
        triBounds.Max.X = std::max(tri.V0.X, std::max(tri.V1.X, tri.V2.X));
        triBounds.Max.Y = std::max(tri.V0.Y, std::max(tri.V1.Y, tri.V2.Y));
        triBounds.Max.Z = std::max(tri.V0.Z, std::max(tri.V1.Z, tri.V2.Z));

        Vector3 triBoundHalfExtent;
        triBoundHalfExtent.X = (triBounds.Max.X - triBounds.Min.X) * 0.5f;
        triBoundHalfExtent.Y = (triBounds.Max.Y - triBounds.Min.Y) * 0.5f;
        triBoundHalfExtent.Z = (triBounds.Max.Z - triBounds.Min.Z) * 0.5f;
        Vector3 triBoundCenter;
        triBoundCenter.X = (triBounds.Max.X + triBounds.Min.X) * 0.5f;
        triBoundCenter.Y = (triBounds.Max.Y + triBounds.Min.Y) * 0.5f;
        triBoundCenter.Z = (triBounds.Max.Z + triBounds.Min.Z) * 0.5f;

        if (triBoundHalfExtent.X + halfExtent.X <= std::abs(triBoundCenter.X) ||
            triBoundHalfExtent.Y + halfExtent.Y <= std::abs(triBoundCenter.Y) ||
            triBoundHalfExtent.Z + halfExtent.Z <= std::abs(triBoundCenter.Z))
            return ContainmentType::Disjoint;

        if (triBoundHalfExtent.X + std::abs(triBoundCenter.X) <= halfExtent.X &&
            triBoundHalfExtent.Y + std::abs(triBoundCenter.Y) <= halfExtent.Y &&
            triBoundHalfExtent.Z + std::abs(triBoundCenter.Z) <= halfExtent.Z)
            return ContainmentType::Contains;

        Vector3 edge1 = tri.V1 - tri.V0;
        Vector3 edge2 = tri.V2 - tri.V0;
        Vector3 edge3 = tri.V1 - tri.V2;

        Vector3 normal = Vector3::Cross(edge1, edge2);
        float triangleDist = Vector3::Dot(tri.V0, normal);
        if (std::abs(normal.X * halfExtent.X) + std::abs(normal.Y * halfExtent.Y) + std::abs(normal.Z * halfExtent.Z) <= std::abs(triangleDist))
            return ContainmentType::Disjoint;

        float dv0, dv1, dv2, dhalf;

        // a.X ^ edge1
        dv0 = tri.V0.Z * edge1.Y - tri.V0.Y * edge1.Z;
        dv1 = tri.V1.Z * edge1.Y - tri.V1.Y * edge1.Z;
        dv2 = tri.V2.Z * edge1.Y - tri.V2.Y * edge1.Z;
        dhalf = std::abs(halfExtent.Y * edge1.Z) + std::abs(halfExtent.Z * edge1.Y);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        // a.X ^ edge2
        dv0 = tri.V0.Z * edge2.Y - tri.V0.Y * edge2.Z;
        dv1 = tri.V1.Z * edge2.Y - tri.V1.Y * edge2.Z;
        dv2 = tri.V2.Z * edge2.Y - tri.V2.Y * edge2.Z;
        dhalf = std::abs(halfExtent.Y * edge2.Z) + std::abs(halfExtent.Z * edge2.Y);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        // a.X ^ edge3
        dv0 = tri.V0.Z * edge3.Y - tri.V0.Y * edge3.Z;
        dv1 = tri.V1.Z * edge3.Y - tri.V1.Y * edge3.Z;
        dv2 = tri.V2.Z * edge3.Y - tri.V2.Y * edge3.Z;
        dhalf = std::abs(halfExtent.Y * edge3.Z) + std::abs(halfExtent.Z * edge3.Y);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        // a.Y ^ edge1
        dv0 = tri.V0.X * edge1.Z - tri.V0.Z * edge1.X;
        dv1 = tri.V1.X * edge1.Z - tri.V1.Z * edge1.X;
        dv2 = tri.V2.X * edge1.Z - tri.V2.Z * edge1.X;
        dhalf = std::abs(halfExtent.X * edge1.Z) + std::abs(halfExtent.Z * edge1.X);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        // a.Y ^ edge2
        dv0 = tri.V0.X * edge2.Z - tri.V0.Z * edge2.X;
        dv1 = tri.V1.X * edge2.Z - tri.V1.Z * edge2.X;
        dv2 = tri.V2.X * edge2.Z - tri.V2.Z * edge2.X;
        dhalf = std::abs(halfExtent.X * edge2.Z) + std::abs(halfExtent.Z * edge2.X);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        // a.Y ^ edge3
        dv0 = tri.V0.X * edge3.Z - tri.V0.Z * edge3.X;
        dv1 = tri.V1.X * edge3.Z - tri.V1.Z * edge3.X;
        dv2 = tri.V2.X * edge3.Z - tri.V2.Z * edge3.X;
        dhalf = std::abs(halfExtent.X * edge3.Z) + std::abs(halfExtent.Z * edge3.X);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        // a.Z ^ edge1
        dv0 = tri.V0.Y * edge1.X - tri.V0.X * edge1.Y;
        dv1 = tri.V1.Y * edge1.X - tri.V1.X * edge1.Y;
        dv2 = tri.V2.Y * edge1.X - tri.V2.X * edge1.Y;
        dhalf = std::abs(halfExtent.Y * edge1.X) + std::abs(halfExtent.X * edge1.Y);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        // a.Z ^ edge2
        dv0 = tri.V0.Y * edge2.X - tri.V0.X * edge2.Y;
        dv1 = tri.V1.Y * edge2.X - tri.V1.X * edge2.Y;
        dv2 = tri.V2.Y * edge2.X - tri.V2.X * edge2.Y;
        dhalf = std::abs(halfExtent.Y * edge2.X) + std::abs(halfExtent.X * edge2.Y);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        // a.Z ^ edge3
        dv0 = tri.V0.Y * edge3.X - tri.V0.X * edge3.Y;
        dv1 = tri.V1.Y * edge3.X - tri.V1.X * edge3.Y;
        dv2 = tri.V2.Y * edge3.X - tri.V2.X * edge3.Y;
        dhalf = std::abs(halfExtent.Y * edge3.X) + std::abs(halfExtent.X * edge3.Y);
        if (std::min(dv0, std::min(dv1, dv2)) >= dhalf || std::max(dv0, std::max(dv1, dv2)) <= -dhalf)
            return ContainmentType::Disjoint;

        return ContainmentType::Intersects;
    }
};

} // namespace CollisionSample
