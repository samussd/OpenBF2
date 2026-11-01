// Ends the plan by reporting the condition's result. [UNRESOLVED] The enum of the three modes:
// 0 stops the plan in failure when the condition holds, 1 and 2 finish a pass with it or its
// negation.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCEND_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCEND_H

#include "BAPFlowControl.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPFlowCEnd : public BAPFlowControl
{
public:
    BAPFlowCEnd(int planId, int mode, BAPCondition* condition);
    virtual ~BAPFlowCEnd();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ BAPCondition* m_condition;
    /* +0x18 */ int m_mode;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCEND_H
