// Behaviour built by BBCreateFunctionRunOverSoldier. sizeof == 0x24.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBRUNOVERSOLDIER_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBRUNOVERSOLDIER_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class PlanGenerator;

class BBRunOverSoldier : public BotBehaviour
{
public:
    BBRunOverSoldier(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBRunOverSoldier();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // A new slot, not an override: the spelling differs from BotBehaviour::isAplicable.
    virtual bool isApplicable(Information* information, Bot* bot);   // +0x34
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBRUNOVERSOLDIER_H
