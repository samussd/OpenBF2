#include "BAPICTurnLock.h"

#include "../AIBot/BotMain.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x208680 (Ghidra). The plan id is left at zero.
BAPICTurnLock::BAPICTurnLock(bool locked)
    : BAPAction(0)
    , m_locked(locked)
{
}

BAPICTurnLock::~BAPICTurnLock()
{
}

bool BAPICTurnLock::execute(Bot* bot, bool& ok, bool& done)
{
    bot->setPlanLock(m_locked);
    ok = true;
    done = true;
    return true;
}

BAPStatementType BAPICTurnLock::getId() const
{
    return BAPSTICTurnLock;
}

bool BAPICTurnLock::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
