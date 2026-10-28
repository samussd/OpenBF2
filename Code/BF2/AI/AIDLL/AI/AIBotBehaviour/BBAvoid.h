// Behaviour built by BBCreateFunctionAvoid. sizeof == 0x24.
// [FRONTIER] Urgency and destruction remain named stops.

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
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBAVOID_H
