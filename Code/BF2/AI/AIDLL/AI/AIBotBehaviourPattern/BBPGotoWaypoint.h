// The goto-waypoint family's interface. No constructor is emitted for it; each subclass
// stores the behaviour id at +8 itself.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPGotoWaypoint : public BBPPattern
{
public:
    virtual void init(Bot* bot) = 0;                                 // +0x18

protected:
    /* +0x08 */ int m_behaviourId;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPGOTOWAYPOINT_H
