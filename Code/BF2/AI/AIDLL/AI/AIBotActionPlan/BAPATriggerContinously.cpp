#include "BAPATriggerContinously.h"

#include "BAPCondition.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1f1a40 (Ghidra).
BAPATriggerContinously::BAPATriggerContinously(io::PlayerInputMap input, BAPCondition* runCondition,
                                               BAPCondition* breakCondition, BAPCondition* third,
                                               unsigned long long controls, int planId, double delay)
    : BAPAction(planId)
    , m_input(input)
    , m_runCondition(runCondition)
    , m_breakCondition(breakCondition)
    , m_third(third)
    , m_pressed(false)
{
    runCondition->m_owner = this;
    if (m_breakCondition)
        m_breakCondition->m_owner = this;
    if (m_third)
        m_third->m_owner = this;
    m_controls = controls;
    m_startTime = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime() + delay;
}

BAPATriggerContinously::~BAPATriggerContinously()
{
    if (m_runCondition)
        delete m_runCondition;
    if (m_breakCondition)
        delete m_breakCondition;
    if (m_third)
        delete m_third;
}

bool BAPATriggerContinously::execute(Bot*, bool& ok, bool& done)
{
    ok = false;
    done = true;
    return false;
}

unsigned long long BAPATriggerContinously::getControls()
{
    return m_controls;
}

BAPStatementType BAPATriggerContinously::getId() const
{
    return BAPSTTriggerContinously;
}

bool BAPATriggerContinously::hasDefault()
{
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
