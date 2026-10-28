// Plan generator for the behaviour of the same family. sizeof == 0x20.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISIONINFANTERY_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISIONINFANTERY_H

#include "BBPAvoidCollision.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPAvoidCollisionInfantery : public BBPAvoidCollision
{
public:
    explicit BBPAvoidCollisionInfantery(int behaviourId);
    virtual ~BBPAvoidCollisionInfantery();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual void init(Bot* bot, Vec3& a, unsigned int handle, Vec3& b, float f);     // +0x18

protected:
    /* +0x1C */ Math::Vec2* m_vec1C;                         // [UNRESOLVED] per bot, zero
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISIONINFANTERY_H
