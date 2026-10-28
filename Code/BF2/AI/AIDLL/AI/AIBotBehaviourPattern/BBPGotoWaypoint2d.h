// Plan generator for the behaviour of the same family. sizeof == 0xC.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT2D_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT2D_H

#include "BBPGotoWaypoint.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPGotoWaypoint2d : public BBPGotoWaypoint
{
public:
    explicit BBPGotoWaypoint2d(int behaviourId);
    virtual ~BBPGotoWaypoint2d();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual void init(Bot* bot);                                     // +0x18
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT2D_H
