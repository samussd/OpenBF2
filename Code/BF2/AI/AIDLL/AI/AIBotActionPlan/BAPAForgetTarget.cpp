#include "BAPAForgetTarget.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/BotSenses.h"
#include "../AIBot/SensingAgent.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1e22e0 (Ghidra). The plan id is left at zero.
BAPAForgetTarget::BAPAForgetTarget(unsigned int target)
    : BAPAction(0)
    , m_target(target)
{
}

BAPAForgetTarget::~BAPAForgetTarget()
{
}

bool BAPAForgetTarget::execute(Bot* bot, bool& ok, bool& done)
{
    bot->getSensingAgent()->getBotSenses()->forgetObject(m_target);
    ok = true;
    done = true;
    return true;
}

BAPStatementType BAPAForgetTarget::getId() const
{
    return BAPSTForgetTarget;
}

bool BAPAForgetTarget::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
