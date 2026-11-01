// True while the bot presses sprint.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONISSPRINTING_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONISSPRINTING_H

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConIsSprinting : public BAPCondition
{
public:
    BAPConIsSprinting() {}
    virtual ~BAPConIsSprinting();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 51; }
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONISSPRINTING_H
