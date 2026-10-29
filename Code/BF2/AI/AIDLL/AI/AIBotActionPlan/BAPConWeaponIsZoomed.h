// True while the active weapon of the bot's equipment is zoomed in.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONISZOOMED_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONISZOOMED_H

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConWeaponIsZoomed : public BAPCondition
{
public:
    BAPConWeaponIsZoomed() {}
    virtual ~BAPConWeaponIsZoomed();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 52; }
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONISZOOMED_H
