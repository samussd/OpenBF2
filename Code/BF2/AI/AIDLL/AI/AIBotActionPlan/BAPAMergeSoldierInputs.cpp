#include "BAPAMergeSoldierInputs.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BAPAMergeSoldierInputs::BAPAMergeSoldierInputs(int planId)
    : BAPAction(planId)
    , m_float20(0.0f)
    , m_float24(0.0f)
{
}

BAPAMergeSoldierInputs::~BAPAMergeSoldierInputs()
{
}

// [FRONTIER] [TEMP] [RETIRE-WHEN: aidll-02-skeleton] 3.2 KB, its own unit.
bool BAPAMergeSoldierInputs::execute(Bot*, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPAMergeSoldierInputs::execute");
}

BAPStatementType BAPAMergeSoldierInputs::getId() const
{
    return BAPSTMergeSoldierInputs;
}

bool BAPAMergeSoldierInputs::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
