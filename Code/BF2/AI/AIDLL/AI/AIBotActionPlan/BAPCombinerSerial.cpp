#include "BAPCombinerSerial.h"

#include "../AIBot/BotMain.h"
#include "../AIPlanInterpreter/PlanInterpreter.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1f6150 (Ghidra).
BAPCombinerSerial::BAPCombinerSerial(int planId, int n)
    : BAPCombiner(planId, true)
    , m_int20(n)
{
    m_statements.clear();
    m_current = m_statements.begin();
}

// 0x1f62b0 (Ghidra).
BAPCombinerSerial::BAPCombinerSerial(int planId, bool waitForAll, int n)
    : BAPCombiner(planId, waitForAll)
    , m_int20(n)
{
    m_statements.clear();
    m_current = m_statements.begin();
}

// 0x1f6410 (Ghidra).
BAPCombinerSerial::BAPCombinerSerial(std::list<BAPStatement*>& statements, int planId, int n)
    : BAPCombiner(planId, true)
    , m_int20(n)
{
    m_statements = statements;
    m_current = m_statements.begin();
    for (std::list<BAPStatement*>::iterator it = m_statements.begin(); it != m_statements.end(); ++it)
        (*it)->m_owner = this;
}

BAPCombinerSerial::~BAPCombinerSerial()
{
    for (std::list<BAPStatement*>::iterator it = m_statements.begin(); it != m_statements.end(); ++it)
    {
        if (*it)
            delete *it;
    }
}

// 0x1f6840 (Ghidra). ok is the statement's; only an empty list sets it here.
bool BAPCombinerSerial::execute(Bot* bot, bool& ok, bool& done)
{
    if (m_current == m_statements.end())
    {
        ok = true;
        done = true;
        m_current = m_statements.begin();
        return true;
    }
    static_cast<BotMain*>(bot)->getPlanInterpreter()->execute(*m_current, bot, ok, done);
    if (m_waitForAll)
    {
        if (!done)
            return true;
        ++m_current;
        if (m_current != m_statements.end())
        {
            done = false;
            return true;
        }
    }
    else
    {
        ++m_current;
        if (!done && m_current != m_statements.end())
            return true;
    }
    done = true;
    m_current = m_statements.begin();
    return true;
}

// No guard: with an empty list retail reads the list's own node. execute keeps the iterator on a
// statement otherwise.
unsigned long long BAPCombinerSerial::getControls()
{
    return (*m_current)->getControls();
}

BAPStatementType BAPCombinerSerial::getId() const
{
    return BAPSTSerial;
}

bool BAPCombinerSerial::hasDefault()
{
    return true;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPCombinerSerial::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPCombinerSerial::streamSelf");
}

void BAPCombinerSerial::addStatement(BAPStatement* statement)
{
    if (!statement)
        return;
    statement->m_owner = this;
    m_statements.push_back(statement);
    m_current = m_statements.begin();
}

} // namespace ai
} // namespace hfe
} // namespace dice
