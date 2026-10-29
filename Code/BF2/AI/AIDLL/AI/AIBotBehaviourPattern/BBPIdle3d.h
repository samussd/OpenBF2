// Plan generator for the behaviour of the same family. sizeof == 0x14.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE3D_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE3D_H

#include "BBPIdle.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPIdle3d : public BBPIdle
{
public:
    explicit BBPIdle3d(int behaviourId);
    virtual ~BBPIdle3d();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual int getType();                                          // +0x18
    // Marks the bot initiated, idling at the target with the value (50 from BBIdle).
    void init(float value, Bot* bot, int target);

protected:
    /* +0x0C */ float* m_float0C;                            // [UNRESOLVED] per bot, 100 at construction; init's value
    /* +0x10 */ int* m_int10;                                // per bot, -1 at construction; init's target
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE3D_H
