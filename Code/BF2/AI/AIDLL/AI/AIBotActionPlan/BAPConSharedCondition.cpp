#include "BAPConSharedCondition.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1f9d80 (Ghidra).
BAPConSharedCondition::BAPConSharedCondition(BAPCondition* condition)
{
    m_shared = new Shared;
    m_shared->m_result = false;
    m_shared->m_holders = 1;
    m_condition = condition;
    condition->m_owner = this;
}

// 0x1f9e60 (Ghidra).
BAPConSharedCondition::BAPConSharedCondition(BAPConSharedCondition& other)
{
    m_condition = 0;
    m_shared = other.m_shared;
    ++m_shared->m_holders;
}

BAPConSharedCondition::~BAPConSharedCondition()
{
    if (m_condition)
        delete m_condition;
    if (--m_shared->m_holders == 0)
        delete m_shared;
}

bool BAPConSharedCondition::evaluate(Bot* bot)
{
    if (m_condition)
        m_shared->m_result = m_condition->evaluate(bot);
    return m_shared->m_result;
}

BAPConSharedCondition* BAPConSharedCondition::getNewSharedCondition()
{
    return new BAPConSharedCondition(*this);
}

} // namespace ai
} // namespace hfe
} // namespace dice
