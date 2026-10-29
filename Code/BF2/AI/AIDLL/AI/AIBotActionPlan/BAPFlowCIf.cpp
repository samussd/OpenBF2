#include "BAPFlowCIf.h"

#include "BAPCondition.h"
#include "../AIBot/BotMain.h"
#include "../AIPlanInterpreter/PlanInterpreter.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Retail does not guard the condition.
BAPFlowCIf::BAPFlowCIf(BAPCondition* condition, BAPStatement* trueStatement,
                       BAPStatement* falseStatement, int planId, bool reevaluate, bool keepFalse)
    : BAPFlowControl(planId)
    , m_condition(condition)
    , m_trueStatement(trueStatement)
    , m_falseStatement(falseStatement)
    , m_activeStatement(0)
    , m_lastActiveStatement(0)
    , m_reevaluate(reevaluate)
    , m_keepFalse(keepFalse)
{
    condition->m_owner = this;
    if (m_trueStatement)
        m_trueStatement->m_owner = this;
    if (m_falseStatement)
        m_falseStatement->m_owner = this;
}

BAPFlowCIf::~BAPFlowCIf()
{
    if (m_condition)
        delete m_condition;
    if (m_trueStatement)
        delete m_trueStatement;
    if (m_falseStatement)
        delete m_falseStatement;
}

// Picks a statement when none runs, or on every step with m_reevaluate (unless the false statement
// runs and m_keepFalse). No statement for the outcome finishes the if at once.
bool BAPFlowCIf::execute(Bot* bot, bool& ok, bool& done)
{
    bool choose;
    if (!m_reevaluate)
        choose = !m_activeStatement;
    else if (m_keepFalse && m_activeStatement == m_falseStatement)
        choose = !m_activeStatement;
    else
        choose = true;

    if (choose)
    {
        m_activeStatement = m_condition->evaluate(bot) ? m_trueStatement : m_falseStatement;
        m_lastActiveStatement = m_activeStatement;
        if (!m_activeStatement)
        {
            ok = true;
            done = true;
            return true;
        }
    }
    static_cast<BotMain*>(bot)->getPlanInterpreter()->execute(m_activeStatement, bot, ok, done);
    if (done)
        m_activeStatement = 0;
    return true;
}

unsigned long long BAPFlowCIf::getControls()
{
    unsigned long long controls = 0;
    if (m_trueStatement)
        controls = m_trueStatement->getControls();
    if (m_falseStatement)
        controls |= m_falseStatement->getControls();
    return controls;
}

BAPStatementType BAPFlowCIf::getId() const
{
    return BAPSTIf;
}

bool BAPFlowCIf::hasDefault()
{
    return true;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPFlowCIf::debug(BotScriptDebugger::DebugLevel, std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPFlowCIf::debug");
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPFlowCIf::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPFlowCIf::streamSelf");
}

BAPCondition* BAPFlowCIf::getCondition()
{
    return m_condition;
}

BAPStatement* BAPFlowCIf::getTrueStatement()
{
    return m_trueStatement;
}

BAPStatement* BAPFlowCIf::getFalseStatement()
{
    return m_falseStatement;
}

BAPStatement* BAPFlowCIf::getActiveStatement()
{
    return m_activeStatement;
}

BAPStatement* BAPFlowCIf::getActiveStatementDebug()
{
    return m_lastActiveStatement;
}

} // namespace ai
} // namespace hfe
} // namespace dice
