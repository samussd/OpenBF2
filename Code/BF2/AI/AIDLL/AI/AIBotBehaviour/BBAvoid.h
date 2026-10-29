// Behaviour built by BBCreateFunctionAvoid. sizeof == 0x24.
// [FRONTIER] Destruction remains a named stop.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBAVOID_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBAVOID_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPAvoidCollision;

class BBAvoid : public BotBehaviour
{
public:
    BBAvoid(unsigned char behaviourId, unsigned char mode, BBPAvoidCollision* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBAvoid();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28

    // Whether a sphere at the relative position, moving at the relative velocity, comes within the
    // radius (squared, times 1.5) of the origin within the look-ahead time: then 1, with the time
    // and the relative position at that contact; otherwise 0. The kind is not read.
    float collisionPredicted(Vec3& relativePosition, Vec3& relativeVelocity, int kind,
                             float lookAhead, float radius, Vec3& contact, float& time);
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBAVOID_H
