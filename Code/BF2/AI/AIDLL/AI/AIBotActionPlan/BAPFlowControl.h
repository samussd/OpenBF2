// The base of the plan's control flow statements (if, while). It adds nothing; retail emits no
// vtable for it.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCONTROL_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCONTROL_H

#include "BAPStatement.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPFlowControl : public BAPStatement
{
protected:
    explicit BAPFlowControl(int planId) : BAPStatement(planId) {}
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCONTROL_H
