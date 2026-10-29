// Plan generator for the behaviour of the same family. sizeof == 0x10.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE2D_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE2D_H

#include "BBPIdle.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPIdle2d : public BBPIdle
{
public:
    explicit BBPIdle2d(int behaviourId);
    virtual ~BBPIdle2d();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual int getType();                                          // +0x18
    // Marks the bot initiated, idling at the target.
    void init(Bot* bot, int target);

protected:
    /* +0x0C */ int* m_int0C;                                // per bot, -1 at construction; init's target
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE2D_H
