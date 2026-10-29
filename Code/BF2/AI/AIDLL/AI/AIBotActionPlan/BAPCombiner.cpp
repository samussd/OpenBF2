#include "BAPCombiner.h"

namespace dice {
namespace hfe {
namespace ai {

BAPCombiner::BAPCombiner(int planId, bool waitForAll)
    : BAPStatement(planId)
    , m_statements()
    , m_current()
    , m_waitForAll(waitForAll)
{
}

// The statements belong to the derived combination, which deletes them first.
BAPCombiner::~BAPCombiner()
{
}

} // namespace ai
} // namespace hfe
} // namespace dice
