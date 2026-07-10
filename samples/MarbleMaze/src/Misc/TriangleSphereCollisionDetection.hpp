#pragma once

// TriangleSphereCollisionDetection.hpp — C++ port of
// Misc/TriangleSphereCollisionDetection.cs (XNA 4.0 MarbleMaze sample). Pure
// Vector3/BoundingSphere/Ray math, ported directly with no CNA gaps. The C#
// `Triangle` reference type becomes a plain value-type struct here (no null
// triangles float around loose in this port -- callers use
// std::optional<Triangle>/std::vector<Triangle> instead, see IntersectDetails.hpp).
//
// C#'s `float.Epsilon` (the smallest positive representable float, ~1.4e-45 --
// NOT machine epsilon) is ported as std::numeric_limits<float>::denorm_min(),
// the equivalent IEEE-754 single-precision value.

#include <limits>
#include <optional>
#include <vector>

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Ray.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::BoundingBox;
using Microsoft::Xna::Framework::BoundingSphere;
using Microsoft::Xna::Framework::ContainmentType;
using Microsoft::Xna::Framework::Ray;
using Microsoft::Xna::Framework::Vector3;

// Represents a simple triangle by the vertices at each corner. Port of the
// C# Triangle class (Misc/TriangleSphereCollisionDetection.cs).
struct Triangle {
    Vector3 A;
    Vector3 B;
    Vector3 C;

    Triangle() = default;
    Triangle(Vector3 v0, Vector3 v1, Vector3 v2) : A(v0), B(v1), C(v2) {}

    // Get a normal that faces away from the point specified (faces in).
    void InverseNormal(const Vector3& point, Vector3& inverseNormal) const {
        Normal(inverseNormal);
        Vector3 inverseDirection = point - A;
        if (Vector3::Dot(inverseNormal, inverseDirection) > 0) {
            inverseNormal = inverseNormal * -1.0f;
        }
    }

    // A unit length vector at right angles to the plane of the triangle.
    void Normal(Vector3& normal) const {
        Vector3 side1 = B - A;
        Vector3 side2 = C - A;
        normal = Vector3::Normalize(Vector3::Cross(side1, side2));
    }
};

// Triangle-Sphere based collision test. Port of the C# static class of the
// same name.
namespace TriangleSphereCollisionDetection {

inline const float epsilon = std::numeric_limits<float>::denorm_min();

// A unit length vector at a right angle to the plane of the triangle.
inline void TriangleNormal(Vector3& normal, const Triangle& triangle) {
    Vector3 side1 = triangle.B - triangle.A;
    Vector3 side2 = triangle.C - triangle.A;
    normal = Vector3::Normalize(Vector3::Cross(side1, side2));
}

// Get a normal that faces towards the triangle from the point given.
inline void TriangleInverseNormal(const Vector3& point, Vector3& inverseNormal, const Triangle& triangle) {
    TriangleNormal(inverseNormal, triangle);
    Vector3 inverseDirection = point - triangle.A;
    if (Vector3::Dot(inverseNormal, inverseDirection) > 0) {
        inverseNormal = inverseNormal * -1.0f;
    }
}

// Determine whether the triangle (v0,v1,v2) intersects the given ray. If there is
// intersection, returns the parametric value of the intersection point on the
// ray. Otherwise returns std::nullopt. Moller-Trumbore.
inline std::optional<float> RayTriangleIntersects(const Ray& ray, const Vector3& v0, const Vector3& v1,
                                                   const Vector3& v2) {
    Vector3 e1 = v1 - v0;
    Vector3 e2 = v2 - v0;

    Vector3 p = Vector3::Cross(ray.Direction, e2);

    float det = Vector3::Dot(e1, p);

    float t;
    if (det >= epsilon) {
        Vector3 s = ray.Position - v0;
        float u = Vector3::Dot(s, p);
        if (u < 0 || u > det)
            return std::nullopt;

        Vector3 q = Vector3::Cross(s, e1);
        float v = Vector3::Dot(ray.Direction, q);
        if (v < 0 || ((u + v) > det))
            return std::nullopt;

        t = Vector3::Dot(e2, q);
        if (t < 0)
            return std::nullopt;
    } else if (det <= -epsilon) {
        Vector3 s = ray.Position - v0;
        float u = Vector3::Dot(s, p);
        if (u > 0 || u < det)
            return std::nullopt;

        Vector3 q = Vector3::Cross(s, e1);
        float v = Vector3::Dot(ray.Direction, q);
        if (v > 0 || ((u + v) < det))
            return std::nullopt;

        t = Vector3::Dot(e2, q);
        if (t > 0)
            return std::nullopt;
    } else {
        // Parallel ray.
        return std::nullopt;
    }

    return t / det;
}

inline std::optional<float> RayTriangleIntersects(const Ray& ray, const Triangle& triangle) {
    return RayTriangleIntersects(ray, triangle.A, triangle.B, triangle.C);
}

// Shoot a ray into the triangle and get the distance from the point it hits; if
// the distance is smaller than the radius we hit the triangle.
inline bool LightSphereTriangleCollision(const BoundingSphere& sphere, const Triangle& triangle) {
    Ray ray;
    ray.Position = sphere.Center;

    Vector3 inverseNormal;
    TriangleInverseNormal(ray.Position, inverseNormal, triangle);
    ray.Direction = inverseNormal;

    std::optional<float> distance = RayTriangleIntersects(ray, triangle);
    if (distance.has_value() && *distance > 0 && *distance <= sphere.Radius) {
        return true;
    }

    return false;
}

// Shoot a ray into the triangle and get the distance from the point it hits (as
// above) and also check collision with the edges.
inline bool SphereTriangleCollision(const BoundingSphere& sphere, const Triangle& triangle) {
    // First check if any corner point is inside the sphere.
    if (sphere.Contains(triangle.A) != ContainmentType::Disjoint ||
        sphere.Contains(triangle.B) != ContainmentType::Disjoint ||
        sphere.Contains(triangle.C) != ContainmentType::Disjoint) {
        return true;
    }

    // Test the edges of the triangle using a ray.
    Vector3 side = triangle.B - triangle.A;
    Ray ray(triangle.A, Vector3::Normalize(side));
    float distSq;
    std::optional<float> length = sphere.Intersects(ray);
    if (length.has_value()) {
        distSq = (*length) * (*length);
        if (*length > 0 && distSq < side.LengthSquared()) {
            return true;
        }
    }

    side = triangle.C - triangle.A;
    ray.Direction = Vector3::Normalize(side);
    length = sphere.Intersects(ray);
    if (length.has_value()) {
        distSq = (*length) * (*length);
        if (*length > 0 && distSq < side.LengthSquared()) {
            return true;
        }
    }

    side = triangle.C - triangle.B;
    ray.Position = triangle.B;
    ray.Direction = Vector3::Normalize(side);
    length = sphere.Intersects(ray);
    if (length.has_value()) {
        distSq = (*length) * (*length);
        if (*length > 0 && distSq < side.LengthSquared()) {
            return true;
        }
    }

    // Calculate the InverseNormal of the triangle from the centre of the sphere
    // and do a ray intersection from the centre of the sphere to the triangle.
    ray.Position = sphere.Center;
    TriangleInverseNormal(ray.Position, side, triangle);
    ray.Direction = side;
    length = RayTriangleIntersects(ray, triangle);
    if (length.has_value() && *length > 0 && *length < sphere.Radius) {
        return true;
    }

    return false;
}

// Check if sphere collides with triangles; returns the (single) first
// triangle it collided with, matching the C# "out Triangle triangle" overload.
inline bool IsSphereCollideWithTriangles(const std::vector<Vector3>& vertices, const BoundingSphere& boundingSphere,
                                         std::optional<Triangle>& triangle, bool light) {
    triangle.reset();

    for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
        Triangle t(vertices[i], vertices[i + 1], vertices[i + 2]);

        bool res = light ? LightSphereTriangleCollision(boundingSphere, t) : SphereTriangleCollision(boundingSphere, t);

        if (res) {
            triangle = t;
            return true;
        }
    }
    return false;
}

// Check if sphere collides with triangles; returns every colliding triangle,
// matching the C# "out IEnumerable<Triangle> triangles" overload.
inline bool IsSphereCollideWithTriangles(const std::vector<Vector3>& vertices, const BoundingSphere& boundingSphere,
                                         std::vector<Triangle>& triangles, bool light) {
    bool res = false;
    triangles.clear();

    for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
        Triangle t(vertices[i], vertices[i + 1], vertices[i + 2]);

        bool tmp = light ? LightSphereTriangleCollision(boundingSphere, t) : SphereTriangleCollision(boundingSphere, t);

        if (tmp) {
            triangles.push_back(t);
            res = true;
        }
    }
    return res;
}

} // namespace TriangleSphereCollisionDetection

} // namespace MarbleMazeSample
