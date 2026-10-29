#include "BAPFlowCWhile.h"

#include "BAPCondition.h"
#include "../AIBot/BotMain.h"
#include "../AIPlanInterpreter/PlanInterpreter.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Retail does not guard the condition.
BAPFlowCWhile::BAPFlowCWhile(BAPCondition* condition, BAPStatement* loopStatement, int planId)
    : BAPFlowControl(planId)
    , m_condition(condition)
    , m_loopStatement(loopStatement)
    , m_break(false)
    , m_testCondition(true)
    , m_testAfterPass(true)
{
    condition->m_owner = this;
    if (m_loopStatement)
        m_loopStatement->m_owner = this;
}

BAPFlowCWhile::~BAPFlowCWhile()
{
    if (m_condition)
        delete m_condition;
    if (m_loopStatement)
        delete m_loopStatement;
}

// Without a statement it reports the condition's negation as done. A loop only finishes through a
// false test or breakLoop; a finished pass alone keeps it running.
bool BAPFlowCWhile::execute(Bot* bot, bool& ok, bool& done)
{
    if (!m_loopStatement)
    {
        ok = true;
        m_break = false;
        done = !m_condition->evaluate(bot);
        return true;
    }
    if (m_break)
    {
        ok = true;
        done = true;
        m_break = false;
        return true;
    }
    if (m_testCondition && !m_condition->evaluate(bot))
    {
        ok = true;
        done = true;
        return true;
    }
    static_cast<BotMain*>(bot)->getPlanInterpreter()->execute(m_loopStatement, bot, ok, done);
    m_testCondition = m_testAfterPass ? done : true;
    done = false;
    return true;
}

unsigned long long BAPFlowCWhile::getControls()
{
    return m_loopStatement ? m_loopStatement->getControls() : 0;
}

BAPStatementType BAPFlowCWhile::getId() const
{
    return BAPSTWhile;
}

bool BAPFlowCWhile::hasDefault()
{
    return true;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPFlowCWhile::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPFlowCWhile::streamSelf");
}

void BAPFlowCWhile::breakLoop()
{
    m_break = true;
}

BAPCondition* BAPFlowCWhile::getCondition()
{
    return m_condition;
}

BAPStatement* BAPFlowCWhile::getLoopStatement()
{
    return m_loopStatement;
}

} // namespace ai
} // namespace hfe
} // namespace dice
