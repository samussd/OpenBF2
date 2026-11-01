// True while the bot may fire.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONALLOWFIRE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONALLOWFIRE_H

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConAllowFire : public BAPCondition
{
public:
    BAPConAllowFire() {}
    virtual ~BAPConAllowFire();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 50; }
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONALLOWFIRE_H
