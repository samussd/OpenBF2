#include "BAPConAllowFire.h"

#include "../AIBot/BotMain.h"

namespace dice {
namespace hfe {
namespace ai {

BAPConAllowFire::~BAPConAllowFire()
{
}

bool BAPConAllowFire::evaluate(Bot* bot)
{
    return bot->isFireAllowed();
}

} // namespace ai
} // namespace hfe
} // namespace dice
