// The information that the bot is idling at a position. While it exists the bot's squad member is
// marked idle.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWIDLE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWIDLE_H

#include "BAPInfoWrapper.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPIWIdle : public BAPInfoWrapper
{
public:
    BAPIWIdle(const Vec3& position, BAPStatement* statement, int planId, Bot* bot);
    virtual ~BAPIWIdle();
    virtual int getInfoType() const;

private:
    /* +0x18 */ Vec3 m_position;
    /* +0x24 */ Bot* m_bot;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWIDLE_H
