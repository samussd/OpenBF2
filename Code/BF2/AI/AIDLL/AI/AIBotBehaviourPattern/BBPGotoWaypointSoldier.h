// Plan generator for MoveTo. The script name BBPGotoWaypointInfantery builds this class.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINTSOLDIER_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINTSOLDIER_H

#include "BBPGotoWaypoint.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPGotoWaypointSoldier : public BBPGotoWaypoint
{
public:
    explicit BBPGotoWaypointSoldier(int behaviourId);
    virtual ~BBPGotoWaypointSoldier();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual void init(Bot* bot);                                     // +0x18
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINTSOLDIER_H
