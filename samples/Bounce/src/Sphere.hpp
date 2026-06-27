#pragma once
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace Bounce {

using namespace Microsoft::Xna::Framework;

struct Sphere {
    Vector3 Position;
    Vector3 Velocity;
    float   Radius      = 0.0f;
    float   Mass        = 0.0f;
    Color   SphereColor = Color(255, 255, 255, 255);
};

} // namespace Bounce
