// Behaviour built by BBCreateFunctionRandom. sizeof == 0x44.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBRANDOM_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBRANDOM_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class PlanGenerator;

class BBRandom : public BotBehaviour
{
public:
    BBRandom(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBRandom();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // A new slot, not an override: the spelling differs from BotBehaviour::isAplicable.
    virtual bool isApplicable(Information* information, Bot* bot);   // +0x34

protected:
    // [INFERRED] names. A flagged grenade warning, shared by every bot: when it was noticed,
    // the fuse time it had left then, and where the grenade is.
    /* +0x24 */ bool m_grenadePending;                   // = false
    /* +0x28 */ double m_grenadeNoticed;                 // = 0
    /* +0x30 */ double m_grenadeTimeLeft;                // = 0
    /* +0x38 */ Vec3 m_grenadePosition;                  // = 0
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBRANDOM_H
