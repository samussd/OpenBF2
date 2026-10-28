// Behaviour built by BBCreateFunctionFireUnarmed, with no pattern, weights or urge curve.
// sizeof == 0x28.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIREUNARMED_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIREUNARMED_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class BBFireUnarmed : public BotBehaviour
{
public:
    BBFireUnarmed(unsigned char behaviourId, unsigned char mode);
    virtual ~BBFireUnarmed();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28

protected:
    /* +0x24 */ float* m_float24;                        // [UNRESOLVED] per bot, zero
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIREUNARMED_H
