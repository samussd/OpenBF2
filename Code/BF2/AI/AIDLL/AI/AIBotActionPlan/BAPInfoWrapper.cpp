#include "BAPInfoWrapper.h"

#include "../AIBot/BotMain.h"
#include "../AIPlanInterpreter/PlanInterpreter.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BAPInfoWrapper::BAPInfoWrapper(BAPStatement* statement, int planId)
    : BAPStatement(planId)
    , m_statement(statement)
{
}

BAPInfoWrapper::~BAPInfoWrapper()
{
}

bool BAPInfoWrapper::execute(Bot* bot, bool& ok, bool& done)
{
    return static_cast<BotMain*>(bot)->getPlanInterpreter()->execute(m_statement, bot, ok, done);
}

unsigned long long BAPInfoWrapper::getControls()
{
    return m_statement->getControls();
}

BAPStatementType BAPInfoWrapper::getId() const
{
    return BAPSTInfoWrapper;
}

bool BAPInfoWrapper::hasDefault()
{
    return true;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPInfoWrapper::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPInfoWrapper::streamSelf");
}

int BAPInfoWrapper::getInfoType() const
{
    return 9;
}

} // namespace ai
} // namespace hfe
} // namespace dice
