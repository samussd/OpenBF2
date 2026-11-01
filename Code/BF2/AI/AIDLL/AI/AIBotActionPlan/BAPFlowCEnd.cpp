#include "BAPFlowCEnd.h"

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1f3230 (Ghidra).
BAPFlowCEnd::BAPFlowCEnd(int planId, int mode, BAPCondition* condition)
    : BAPFlowControl(planId)
    , m_condition(condition)
    , m_mode(mode)
{
    if (condition)
        condition->m_owner = this;
}

BAPFlowCEnd::~BAPFlowCEnd()
{
    if (m_condition)
        delete m_condition;
}

// 0x1f3460 (Ghidra). Other modes leave both results as they are.
bool BAPFlowCEnd::execute(Bot* bot, bool& ok, bool& done)
{
    switch (m_mode)
    {
    case 0:
        if (m_condition && !m_condition->evaluate(bot))
        {
            done = false;
            ok = true;
            return true;
        }
        done = true;
        ok = false;
        return true;
    case 1:
        if (!m_condition)
        {
            done = true;
            ok = false;
            return true;
        }
        done = m_condition->evaluate(bot);
        ok = true;
        return true;
    case 2:
        if (!m_condition)
        {
            done = false;
            ok = false;
            return true;
        }
        done = !m_condition->evaluate(bot);
        ok = true;
        return true;
    default:
        return true;
    }
}

unsigned long long BAPFlowCEnd::getControls()
{
    return 0;
}

BAPStatementType BAPFlowCEnd::getId() const
{
    return BAPSTPlanEnd;
}

bool BAPFlowCEnd::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
