// The information that the bot is firing at a target. While it exists the bot counts as firing and
// has that firing target; removing it clears what the plan set up for the attack.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWFIRE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWFIRE_H

#include "BAPInfoWrapper.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPIWFire : public BAPInfoWrapper
{
public:
    BAPIWFire(Bot* bot, unsigned int target, bool b1, bool b2, int i1, int i2, BAPStatement* statement,
              int planId);
    virtual ~BAPIWFire();
    virtual int getInfoType() const;

private:
    /* +0x18 */ unsigned int m_target;
    /* +0x1C */ int m_int1C;                // [UNRESOLVED]
    /* +0x20 */ int m_int20;                // [UNRESOLVED]
    /* +0x24 */ bool m_bool24;              // [UNRESOLVED]
    /* +0x25 */ bool m_bool25;              // [UNRESOLVED]
    /* +0x28 */ int m_int28;                // [UNRESOLVED] = -1
    /* +0x2C */ Bot* m_bot;
    /* +0x30 */ bool m_useStaticAttack;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPIWFIRE_H
