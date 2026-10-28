// Plan generator for the behaviour of the same family. sizeof == 0x18.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPC4_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPC4_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPC4 : public BBPPattern
{
public:
    explicit BBPC4(int behaviourId);
    virtual ~BBPC4();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08

protected:
    /* +0x08 */ int m_behaviourId;
    /* +0x0C */ int* m_int0C;                                // [UNRESOLVED] per bot, not initialised
    /* +0x10 */ int m_int10[2];                              // [UNRESOLVED] not written by the constructor
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPC4_H
