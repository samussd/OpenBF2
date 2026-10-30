#include "BAPIWMoveTo.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x2077a0 (Ghidra). Retail does not guard the statement.
BAPIWMoveTo::BAPIWMoveTo(const Vec3& position, BAPStatement* statement, int planId)
    : BAPInfoWrapper(statement, planId)
    , m_position(position)
{
    m_statement->m_owner = this;
}

BAPIWMoveTo::~BAPIWMoveTo()
{
    if (m_statement)
        delete m_statement;
}

int BAPIWMoveTo::getInfoType() const
{
    return 2;
}

} // namespace ai
} // namespace hfe
} // namespace dice
