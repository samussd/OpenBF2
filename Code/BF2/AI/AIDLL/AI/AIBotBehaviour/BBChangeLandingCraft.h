// Behaviour built by BBCreateFunctionChangeLandingCraft. sizeof == 0x24.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBCHANGELANDINGCRAFT_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBCHANGELANDINGCRAFT_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class PlanGenerator;

class BBChangeLandingCraft : public BotBehaviour
{
public:
    BBChangeLandingCraft(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBChangeLandingCraft();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBCHANGELANDINGCRAFT_H
