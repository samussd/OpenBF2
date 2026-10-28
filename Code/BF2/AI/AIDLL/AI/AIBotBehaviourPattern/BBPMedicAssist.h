// Plan generator for the behaviour of the same family. sizeof == 0x18.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPMEDICASSIST_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPMEDICASSIST_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPMedicAssist : public BBPPattern
{
public:
    explicit BBPMedicAssist(int behaviourId);
    virtual ~BBPMedicAssist();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08

protected:
    /* +0x08 */ int m_behaviourId;
    /* +0x0C */ int* m_int0C;                                // [UNRESOLVED] per bot, -1
    /* +0x10 */ int* m_int10;                                // [UNRESOLVED] per bot, -1
    /* +0x14 */ int* m_int14;                                // [UNRESOLVED] per bot, -1
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPMEDICASSIST_H
