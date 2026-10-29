#include "BAPATrigger.h"

#include "BAPCondition.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice {
namespace hfe {
namespace ai {

BAPATrigger::BAPATrigger(io::PlayerInputMap input, BAPCondition* runCondition,
                         BAPCondition* breakCondition, unsigned long long controls, int planId,
                         double delay, bool b)
    : BAPAction(planId)
    , m_input(input)
    , m_runCondition(runCondition)
    , m_breakCondition(breakCondition)
    , m_bool20(false)
    , m_bool2C(b)
{
    if (runCondition)
        runCondition->m_owner = this;
    if (m_breakCondition)
        m_breakCondition->m_owner = this;
    m_controls = controls;
    m_startTime = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime() + delay;
}

BAPATrigger::~BAPATrigger()
{
    if (m_runCondition)
        delete m_runCondition;
    if (m_breakCondition)
        delete m_breakCondition;
}

bool BAPATrigger::execute(Bot*, bool& ok, bool& done)
{
    ok = false;
    done = true;
    return false;
}

unsigned long long BAPATrigger::getControls()
{
    return m_controls;
}

BAPStatementType BAPATrigger::getId() const
{
    return BAPSTTrigger;
}

bool BAPATrigger::hasDefault()
{
    return false;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPATrigger::debug(BotScriptDebugger::DebugLevel, std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPATrigger::debug");
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPATrigger::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPATrigger::streamSelf");
}

} // namespace ai
} // namespace hfe
} // namespace dice
