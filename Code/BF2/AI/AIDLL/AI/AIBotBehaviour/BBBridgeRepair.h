// Behaviour built by BBCreateFunctionBridgeRepair. sizeof == 0x3C.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBBRIDGEREPAIR_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBBRIDGEREPAIR_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class PlanGenerator;

class BBBridgeRepair : public BotBehaviour
{
public:
    BBBridgeRepair(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBBridgeRepair();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // A new slot, not an override: the spelling differs from BotBehaviour::isAplicable.
    virtual bool isApplicable(Information* information, Bot* bot);   // +0x34

protected:
    /* +0x24 */ int m_words24[3];                        // [UNRESOLVED] three zero words
    /* +0x30 */ int* m_int30;                            // [UNRESOLVED] one per battle strength
    /* +0x34 */ int* m_int34;                            // [UNRESOLVED] one per battle strength
    /* +0x38 */ int* m_int38;                            // [UNRESOLVED] one per battle strength
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBBRIDGEREPAIR_H
