// True while the bot is within a distance of a point, optionally ignoring height.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONPOINTDISTANCE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONPOINTDISTANCE_H

#include "BAPCondition.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConPointDistance : public BAPCondition
{
public:
    BAPConPointDistance(const Vec3& point, float distance, bool flat);
    virtual ~BAPConPointDistance();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 12; }

private:
    /* +0x08 */ float m_minSqr;
    /* +0x0C */ float m_maxSqr;
    /* +0x10 */ bool m_flat;
    /* +0x14 */ float m_lastSqr;
    /* +0x18 */ Vec3 m_point;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONPOINTDISTANCE_H
