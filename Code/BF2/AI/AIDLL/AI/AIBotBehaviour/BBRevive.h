// Behaviour built by BBCreateFunctionRevive. sizeof == 0x3C.
// [FRONTIER] Destruction remains a named stop.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBREVIVE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBREVIVE_H

#include "BotBehaviour.h"

#include <vector>

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
    /* +0x24 */ std::vector<unsigned int> m_candidates;  // the friendly units of the last evaluation
    // Per battle strength, the bot's best revive tool against it: [INFERRED] names.
    /* +0x30 */ int* m_toolIndex;                        // index in the armament, -1 for none
    /* +0x34 */ float* m_toolRangeSquared;               // its maximum range squared
    /* +0x38 */ int* m_toolStrength;                     // its strength
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBREVIVE_H
