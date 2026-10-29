#include "BAPStatement.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

int BAPStatement::news = 0;
int BAPStatement::destroys = 0;

BotScript::~BotScript()
{
}

BAPStatement::BAPStatement(int planId)
    : m_controls(0)
    , m_planId(planId)
{
    ++news;
}

BAPStatement::~BAPStatement()
{
    ++destroys;
}

unsigned long long BAPStatement::getControls()
{
    return m_controls;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPStatement::debug(BotScriptDebugger::DebugLevel, std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPStatement::debug");
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPStatement::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPStatement::streamSelf");
}

} // namespace ai
} // namespace hfe
} // namespace dice
