#include "BAPAIdle.h"

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1e2850 (Ghidra).
BAPAIdle::BAPAIdle(BAPCondition* condition, unsigned long long controls, int planId)
    : BAPAction(planId)
    , m_condition(condition)
{
    condition->m_owner = this;
    m_controls = controls;
}

BAPAIdle::~BAPAIdle()
{
    if (m_condition)
        delete m_condition;
}

bool BAPAIdle::execute(Bot* bot, bool& ok, bool& done)
{
    ok = true;
    done = m_condition->evaluate(bot);
    return true;
}

unsigned long long BAPAIdle::getControls()
{
    return m_controls;
}

BAPStatementType BAPAIdle::getId() const
{
    return BAPSTIdle;
}

bool BAPAIdle::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
