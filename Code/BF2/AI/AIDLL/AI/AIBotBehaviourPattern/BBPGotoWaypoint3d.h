// Plan generator for the behaviour of the same family. sizeof == 0xC.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT3D_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT3D_H

#include "BBPGotoWaypoint.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPGotoWaypoint3d : public BBPGotoWaypoint
{
public:
    explicit BBPGotoWaypoint3d(int behaviourId);
    virtual ~BBPGotoWaypoint3d();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual void init(Bot* bot);                                     // +0x18
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT3D_H
