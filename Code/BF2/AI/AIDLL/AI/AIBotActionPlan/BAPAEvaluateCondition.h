// Evaluates a condition once. A BAPConSharedCondition keeps the result for other statements.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAEVALUATECONDITION_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAEVALUATECONDITION_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPAEvaluateCondition : public BAPAction
{
public:
    BAPAEvaluateCondition(BAPCondition* condition, int planId);
    virtual ~BAPAEvaluateCondition();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ BAPCondition* m_condition;
    /* +0x18 */ bool m_result;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAEVALUATECONDITION_H
