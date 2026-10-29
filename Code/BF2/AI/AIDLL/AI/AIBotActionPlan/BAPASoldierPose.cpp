#include "BAPASoldierPose.h"

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

// One pose for all three situations, a fire threshold of 2, no control threshold, no target and
// five seconds. The statement drives inputs 37 and 38 (0x60 in the high word). Retail does not
// guard the run condition.
BAPASoldierPose::BAPASoldierPose(SoldierPose pose, BAPCondition* runCondition, int planId, bool b)
    : BAPAction(planId)
    , BAPAComponentSoldierPose(pose, pose, 2.0f, pose, 0.0f, 0xFFFFFFFFu, 5.0f)
    , m_runCondition(runCondition)
    , m_breakCondition(0)
    , m_bool48(b)
{
    runCondition->m_owner = static_cast<BAPAction*>(this);
    m_controls = 0x60ULL << 32;
}

BAPASoldierPose::~BAPASoldierPose()
{
    if (m_runCondition)
        delete m_runCondition;
    if (m_breakCondition)
        delete m_breakCondition;
}

bool BAPASoldierPose::execute(Bot*, bool& ok, bool& done)
{
    ok = false;
    done = true;
    return false;
}

BAPStatementType BAPASoldierPose::getId() const
{
    return BAPSTSoldierPose;
}

bool BAPASoldierPose::hasDefault()
{
    return false;
}

BAPCondition* BAPASoldierPose::getRunCondition()
{
    return m_runCondition;
}

BAPCondition* BAPASoldierPose::getBreakCondition()
{
    return m_breakCondition;
}

} // namespace ai
} // namespace hfe
} // namespace dice
