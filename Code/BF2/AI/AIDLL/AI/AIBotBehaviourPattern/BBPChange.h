// Plan generator for the behaviour of the same family. sizeof == 0x1C.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPCHANGE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPCHANGE_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPChange : public BBPPattern
{
public:
    explicit BBPChange(int behaviourId);
    virtual ~BBPChange();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08

protected:
    /* +0x08 */ int m_behaviourId;
    /* +0x0C */ int* m_int0C;                                // [UNRESOLVED] per bot, not initialised
    /* +0x10 */ Vec3* m_vec10;                               // [UNRESOLVED] per bot, zero
    /* +0x14 */ bool* m_bool14;                              // [UNRESOLVED] per bot, not initialised
    /* +0x18 */ bool* m_bool18;                              // [UNRESOLVED] per bot, not initialised
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPCHANGE_H
