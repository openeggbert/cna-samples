#pragma once

namespace Flocking {

struct AIParameters {
    float DetectionDistance;
    float SeparationDistance;
    float MoveInOldDirectionInfluence;
    float MoveInFlockDirectionInfluence;
    float MoveInRandomDirectionInfluence;
    float MaxTurnRadians;
    float PerMemberWeight;
    float PerDangerWeight;
};

} // namespace Flocking
