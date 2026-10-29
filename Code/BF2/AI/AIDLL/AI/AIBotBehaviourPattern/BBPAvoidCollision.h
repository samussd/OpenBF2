// The avoid-collision family's base. sizeof == 0x1C.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPAvoidCollision : public BBPPattern
{
public:
    BBPAvoidCollision();
    virtual ~BBPAvoidCollision();

    // Marks the bot initiated to keep clear of the obstacle: where it will be against the bot at
    // contact, its velocity against the bot and the time to contact (BBAvoid's arguments).
    virtual void init(Bot* bot, Vec3& contact, unsigned int obstacle, Vec3& relativeVelocity,
                      float time) = 0;                                                     // +0x18

protected:
    /* +0x08 */ int m_behaviourId;                           // the subclasses store it
    // Per bot, what init was given: [INFERRED] names.
    /* +0x0C */ unsigned int* m_obstacle;                    // not initialised
    /* +0x10 */ Vec3* m_relativeVelocity;                    // zero
    /* +0x14 */ float* m_time;                               // not initialised
    /* +0x18 */ int m_int18;                                 // [UNRESOLVED] not written by the constructor
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION_H
