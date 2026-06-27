#pragma once
#include "Microsoft/Xna/Framework/GameTime.hpp"

namespace WaypointSample {

class Tank;

class Behavior {
protected:
    Tank& tank_;
public:
    explicit Behavior(Tank& tank);
    virtual ~Behavior() = default;
    virtual void Update(const GameTime& gameTime) = 0;
};

} // namespace WaypointSample
