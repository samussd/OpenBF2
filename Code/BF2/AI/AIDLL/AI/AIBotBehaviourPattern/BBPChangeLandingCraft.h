// Plan generator for the behaviour of the same family. sizeof == 0x14.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPCHANGELANDINGCRAFT_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPCHANGELANDINGCRAFT_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPChangeLandingCraft : public BBPPattern
{
public:
    explicit BBPChangeLandingCraft(int behaviourId);
    virtual ~BBPChangeLandingCraft();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08

protected:
    /* +0x08 */ int m_behaviourId;
    /* +0x0C */ int* m_int0C;                                // [UNRESOLVED] per bot, not initialised
    /* +0x10 */ bool* m_bool10;                              // [UNRESOLVED] per bot, not initialised
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPCHANGELANDINGCRAFT_H
