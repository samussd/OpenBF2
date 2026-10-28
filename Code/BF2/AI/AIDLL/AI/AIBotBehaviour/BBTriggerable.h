// Behaviour built by BBCreateFunctionTriggerable. sizeof == 0x28.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBTRIGGERABLE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBTRIGGERABLE_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class PlanGenerator;

class BBTriggerable : public BotBehaviour
{
public:
    BBTriggerable(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBTriggerable();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // A new slot, not an override: the spelling differs from BotBehaviour::isAplicable.
    virtual bool isApplicable(Information* information, Bot* bot);   // +0x34

protected:
    /* +0x24 */ int m_int24;                             // [UNRESOLVED] zero
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBTRIGGERABLE_H
