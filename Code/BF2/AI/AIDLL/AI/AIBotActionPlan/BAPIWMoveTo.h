// The information that the bot is moving to a position. The move generator reads the position back
// to decide whether its plan still holds.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWMOVETO_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWMOVETO_H

#include "BAPInfoWrapper.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPIWMoveTo : public BAPInfoWrapper
{
public:
    BAPIWMoveTo(const Vec3& position, BAPStatement* statement, int planId);
    virtual ~BAPIWMoveTo();
    virtual int getInfoType() const;

    // [CHOSEN] accessor; BBPGotoWaypointSoldier::createPlan reads the field directly.
    const Vec3& getPosition() const { return m_position; }

private:
    /* +0x18 */ Vec3 m_position;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWMOVETO_H
