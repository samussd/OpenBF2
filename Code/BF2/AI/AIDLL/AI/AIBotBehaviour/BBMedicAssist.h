// Behaviour built by BBCreateFunctionMedicAssist. sizeof == 0x3C.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMEDICASSIST_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMEDICASSIST_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class PlanGenerator;

class BBMedicAssist : public BotBehaviour
{
public:
    BBMedicAssist(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBMedicAssist();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // A bot that is not down and carries a weapon of type 1 or 2.
    virtual bool isAplicable(Information* information, Bot* bot);   // +0x30

protected:
    /* +0x24 */ int m_words24[3];                        // [UNRESOLVED] three zero words
    /* +0x30 */ int* m_int30;                            // [UNRESOLVED] one per battle strength
    /* +0x34 */ int* m_int34;                            // [UNRESOLVED] one per battle strength
    /* +0x38 */ int* m_int38;                            // [UNRESOLVED] one per battle strength
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMEDICASSIST_H
