// Plan generator for the behaviour of the same family. sizeof == 0x20.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION2D_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION2D_H

#include "BBPAvoidCollision.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPAvoidCollision2d : public BBPAvoidCollision
{
public:
    explicit BBPAvoidCollision2d(int behaviourId);
    virtual ~BBPAvoidCollision2d();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual void init(Bot* bot, Vec3& contact, unsigned int obstacle, Vec3& relativeVelocity,
                      float time);                                                         // +0x18

protected:
    /* +0x1C */ Math::Vec2* m_vec1C;                         // [UNRESOLVED] per bot, zero
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION2D_H
