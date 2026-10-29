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

    // Marks the bot initiated, to treat the object behind the handle with the weapon at the
    // index in its armament; the player occupying it, -1 for none.
    void init(unsigned int target, int toolIndex, int occupant, Bot* bot);

protected:
    /* +0x08 */ int m_behaviourId;
    // Per bot, -1 at construction: [INFERRED] names.
    /* +0x0C */ unsigned int* m_target;
    /* +0x10 */ int* m_toolIndex;
    /* +0x14 */ int* m_occupant;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPMEDICASSIST_H
