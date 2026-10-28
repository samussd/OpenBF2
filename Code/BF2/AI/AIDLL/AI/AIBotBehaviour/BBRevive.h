// Behaviour built by BBCreateFunctionRevive. sizeof == 0x3C.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBREVIVE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBREVIVE_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPRevive;

class BBRevive : public BotBehaviour
{
public:
    BBRevive(unsigned char behaviourId, unsigned char mode, BBPRevive* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBRevive();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // A kit carrying a weapon of type 3.
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

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBREVIVE_H
