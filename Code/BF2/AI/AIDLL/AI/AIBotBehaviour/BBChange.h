// Behaviour built by BBCreateFunctionChange. sizeof == 0x44.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBCHANGE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBCHANGE_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class PlanGenerator;

class BBChange : public BotBehaviour
{
public:
    BBChange(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBChange();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28

protected:
    /* +0x24 */ int m_words24[8];                        // [UNRESOLVED] eight zero words
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBCHANGE_H
