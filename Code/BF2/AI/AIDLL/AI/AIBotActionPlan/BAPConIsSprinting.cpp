#include "BAPConIsSprinting.h"

#include "../AIBot/BotMain.h"
#include "../../../../IO/Input/PlayerInput.h"

namespace dice {
namespace hfe {
namespace ai {

BAPConIsSprinting::~BAPConIsSprinting()
{
}

// 0x1f7550 (Ghidra).
bool BAPConIsSprinting::evaluate(Bot* bot)
{
    return bot->getPlayerInput()->getInput(io::PISprint) != 0.0f;
}

} // namespace ai
} // namespace hfe
} // namespace dice
