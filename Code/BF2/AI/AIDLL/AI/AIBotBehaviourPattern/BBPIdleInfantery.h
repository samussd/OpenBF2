// Plan generator for the behaviour of the same family. sizeof == 0xC.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLEINFANTERY_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLEINFANTERY_H

#include "BBPIdle.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPIdleInfantery : public BBPIdle
{
public:
    explicit BBPIdleInfantery(int behaviourId);
    virtual ~BBPIdleInfantery();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual int getType();                                          // +0x18
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLEINFANTERY_H
