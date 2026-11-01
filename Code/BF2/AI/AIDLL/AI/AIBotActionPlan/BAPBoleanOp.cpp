#include "BAPBoleanOp.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// The operands belong to the derived operation, which deletes them first.
BAPBoleanOp::~BAPBoleanOp()
{
}

BAPConNot::BAPConNot(BAPCondition* condition)
{
    addCondition(condition);
}

BAPConNot::~BAPConNot()
{
    for (std::list<BAPCondition*>::iterator it = m_conditions.begin(); it != m_conditions.end(); ++it)
    {
        if (*it)
            delete *it;
    }
}

// Retail reads the first operand without checking that there is one.
bool BAPConNot::evaluate(Bot* bot)
{
    return !m_conditions.front()->evaluate(bot);
}

void BAPConNot::addCondition(BAPCondition* condition)
{
    if (!m_conditions.empty())
        return;
    condition->m_owner = this;
    m_conditions.push_back(condition);
}

BAPConAnd::~BAPConAnd()
{
    for (std::list<BAPCondition*>::iterator it = m_conditions.begin(); it != m_conditions.end(); ++it)
    {
        if (*it)
            delete *it;
    }
}

bool BAPConAnd::evaluate(Bot* bot)
{
    for (std::list<BAPCondition*>::iterator it = m_conditions.begin(); it != m_conditions.end(); ++it)
    {
        if (!(*it)->evaluate(bot))
            return false;
    }
    return true;
}

void BAPConAnd::addCondition(BAPCondition* condition)
{
    condition->m_owner = this;
    m_conditions.push_back(condition);
}

BAPConOr::~BAPConOr()
{
    for (std::list<BAPCondition*>::iterator it = m_conditions.begin(); it != m_conditions.end(); ++it)
    {
        if (*it)
            delete *it;
    }
}

bool BAPConOr::evaluate(Bot* bot)
{
    for (std::list<BAPCondition*>::iterator it = m_conditions.begin(); it != m_conditions.end(); ++it)
    {
        if ((*it)->evaluate(bot))
            return true;
    }
    return false;
}

void BAPConOr::addCondition(BAPCondition* condition)
{
    condition->m_owner = this;
    m_conditions.push_back(condition);
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPConNot::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPConNot::streamSelf");
}

} // namespace ai
} // namespace hfe
} // namespace dice
