// Behaviour built by BBCreateFunctionMoveTo. sizeof == 0x24.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMOVETO_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMOVETO_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPGotoWaypoint;

class BBMoveTo : public BotBehaviour
{
public:
    BBMoveTo(unsigned char behaviourId, unsigned char mode, BBPGotoWaypoint* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBMoveTo();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMOVETO_H
