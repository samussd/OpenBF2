#include "BAPAEvaluateCondition.h"

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1e1ed0 (Ghidra).
BAPAEvaluateCondition::BAPAEvaluateCondition(BAPCondition* condition, int planId)
    : BAPAction(planId)
    , m_condition(condition)
    , m_result(false)
{
    condition->m_owner = this;
}

BAPAEvaluateCondition::~BAPAEvaluateCondition()
{
    if (m_condition)
        delete m_condition;
}

bool BAPAEvaluateCondition::execute(Bot* bot, bool& ok, bool& done)
{
    m_result = m_condition->evaluate(bot);
    ok = true;
    done = true;
    return true;
}

unsigned long long BAPAEvaluateCondition::getControls()
{
    return 0;
}

BAPStatementType BAPAEvaluateCondition::getId() const
{
    return BAPSTEvaluateCondition;
}

bool BAPAEvaluateCondition::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
