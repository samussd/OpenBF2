#include "BAPCombinerParallel.h"

#include "../AIBot/BotMain.h"
#include "../AIPlanInterpreter/PlanInterpreter.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BAPCombinerParallel::BAPCombinerParallel(int planId, int n, bool neverFinish)
    : BAPCombiner(planId, true)
    , m_isFinished()
    , m_int24(n)
    , m_neverFinish(neverFinish)
{
}

BAPCombinerParallel::BAPCombinerParallel(int planId, bool waitForAll, int n, bool neverFinish)
    : BAPCombiner(planId, waitForAll)
    , m_isFinished()
    , m_int24(n)
    , m_neverFinish(neverFinish)
{
}

BAPCombinerParallel::~BAPCombinerParallel()
{
    for (std::list<BAPStatement*>::iterator it = m_statements.begin(); it != m_statements.end(); ++it)
    {
        if (*it)
            delete *it;
    }
}

// Runs every statement once. ok holds while every statement could run. done starts at
// m_waitForAll and then ands (waiting for all) or ors (any) each statement's done.
// Each statement's controls are asked for and dropped, as shipped. Retail keeps the per-statement
// flags iterator on the first flag, so every statement's done lands in that one flag.
// The statements share one pair of results that is not reset between them.
bool BAPCombinerParallel::execute(Bot* bot, bool& ok, bool& done)
{
    bool statementOk = true;
    bool statementDone = false;
    done = m_waitForAll;
    ok = true;
    PlanInterpreter* const interpreter = static_cast<BotMain*>(bot)->getPlanInterpreter();
    const std::list<bool>::iterator finished = m_isFinished.begin();
    for (m_current = m_statements.begin(); m_current != m_statements.end(); ++m_current)
    {
        (*m_current)->getControls();
        interpreter->execute(*m_current, bot, statementOk, statementDone);
        ok = ok && statementOk;
        done = m_waitForAll ? (done && statementDone) : (done || statementDone);
        *finished = statementDone;
    }
    if (m_neverFinish)
        done = false;
    return true;
}

unsigned long long BAPCombinerParallel::getControls()
{
    return m_controls;
}

BAPStatementType BAPCombinerParallel::getId() const
{
    return BAPSTParallel;
}

bool BAPCombinerParallel::hasDefault()
{
    return true;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPCombinerParallel::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPCombinerParallel::streamSelf");
}

void BAPCombinerParallel::addStatement(BAPStatement* statement)
{
    if (!statement)
        return;
    statement->m_owner = this;
    m_statements.push_back(statement);
    m_controls |= statement->getControls();
    m_isFinished.push_back(false);
}

std::list<bool>& BAPCombinerParallel::getIsFinished()
{
    return m_isFinished;
}

} // namespace ai
} // namespace hfe
} // namespace dice
