#pragma once

// IntersectDetails.hpp — C++ port of Misc/IntersectDetails.cs (XNA 4.0 MarbleMaze
// sample). Triangle is a reference type in C# (so IntersectedGroundTriangle can be
// null); ported as std::optional<Triangle> here. The two IEnumerable<Triangle>
// fields (populated from TriangleSphereCollisionDetection::IsSphereCollideWithTriangles's
// "out IEnumerable<Triangle>" overload, which always builds a concrete List<Triangle>
// internally) are ported as std::vector<Triangle>.

#include <optional>
#include <vector>

#include "TriangleSphereCollisionDetection.hpp"

namespace MarbleMazeSample {

struct IntersectDetails {
    bool IntersectWithGround = false;
    bool IntersectWithFloorSides = false;
    bool IntersectWithWalls = false;

    std::optional<Triangle> IntersectedGroundTriangle;
    std::vector<Triangle> IntersectedFloorSidesTriangle;
    std::vector<Triangle> IntersectedWallTriangle;
};

} // namespace MarbleMazeSample
