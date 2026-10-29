// Plan generator for the behaviour of the same family. sizeof == 0x18.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPC4_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPC4_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class AIStrategicObject;

class BBPC4 : public BBPPattern
{
public:
    explicit BBPC4(int behaviourId);
    virtual ~BBPC4();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    // Marks the bot initiated, to detonate its charge or else place one, for the object.
    void init(Bot* bot, bool detonate, AIStrategicObject* target);

protected:
    /* +0x08 */ int m_behaviourId;
    /* +0x0C */ int* m_int0C;                                // [UNRESOLVED] per bot, not initialised
    // [INFERRED] names; init writes them, the constructor does not.
    /* +0x10 */ bool m_detonate;
    /* +0x14 */ AIStrategicObject* m_target;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPC4_H
