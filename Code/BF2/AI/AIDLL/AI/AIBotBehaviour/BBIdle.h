// Behaviour built by BBCreateFunctionIdle. sizeof == 0x30.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBIDLE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBIDLE_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPIdle;

class BBIdle : public BotBehaviour
{
public:
    BBIdle(unsigned char behaviourId, unsigned char mode, BBPIdle* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBIdle();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // Clears every bot's state, not only this one's, and leaves the pattern alone.
    virtual void reset(Bot* bot);                                    // +0x24

protected:
    /* +0x24 */ double* m_double24;                      // [UNRESOLVED] per bot, zero
    /* +0x28 */ bool* m_bool28;                          // [UNRESOLVED] per bot, false
    /* +0x2C */ float* m_float2C;                        // [UNRESOLVED] per bot, zero
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBIDLE_H
