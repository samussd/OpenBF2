#include "BAPAOverrideSensingAgent.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/SensingAgent.h"

namespace dice {
namespace hfe {
namespace ai {

BAPAOverrideSensingAgent::~BAPAOverrideSensingAgent()
{
}

// 0x1edb90 (Ghidra).
bool BAPAOverrideSensingAgent::execute(Bot* bot, bool& ok, bool& done)
{
    bot->getSensingAgent()->setOverride(true);
    ok = true;
    done = true;
    return false;
}

BAPStatementType BAPAOverrideSensingAgent::getId() const
{
    return BAPSTOverrideSensingAgent;
}

bool BAPAOverrideSensingAgent::hasDefault()
{
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
