// Plan generator for the behaviour of the same family. sizeof == 0x14.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPREVIVE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPREVIVE_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPRevive : public BBPPattern
{
public:
    explicit BBPRevive(int behaviourId);
    virtual ~BBPRevive();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08

    // Marks the bot initiated, to revive the soldier behind the handle with the weapon at the
    // index in its armament.
    void init(unsigned int target, int toolIndex, Bot* bot);

protected:
    /* +0x08 */ int m_behaviourId;
    // [INFERRED] names; init writes them, the constructor does not.
    /* +0x0C */ unsigned int* m_target;                      // per bot
    /* +0x10 */ int* m_toolIndex;                            // per bot
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPREVIVE_H
