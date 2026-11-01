// True when the bot is within a radius of a position. A Vec2 position ignores height, and the
// Vec3 form also counts a position exactly on the radius. A bot with no equipment counts as there.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONPOSITION_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONPOSITION_H

#include "BAPCondition.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConPosition : public BAPCondition
{
public:
    BAPConPosition(const Vec3& position, float radius);
    BAPConPosition(const Vec2& position, float radius);
    virtual ~BAPConPosition();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 37; }

private:
    /* +0x08 */ float* m_position;      // three or two floats, by m_hasHeight
    /* +0x0C */ float m_radiusSqr;
    /* +0x10 */ bool m_hasHeight;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONPOSITION_H
