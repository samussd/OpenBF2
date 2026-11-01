#include "BAPConWeaponHasLockOnTarget.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x202310 (Ghidra).
BAPConWeaponHasLockOnTarget::BAPConWeaponHasLockOnTarget(BAPVariable<WeaponInfo*>* weapon, unsigned int target)
    : m_weapon(weapon)
    , m_target(target)
{
}

BAPConWeaponHasLockOnTarget::~BAPConWeaponHasLockOnTarget()
{
}

// 0x202410 (Ghidra).
bool BAPConWeaponHasLockOnTarget::evaluate(Bot* bot)
{
    IAIObject* const locked = m_weapon->m_value->getLockedTarget();
    if (!locked)
        return false;
    return informationManager->getInformation(locked->getInformation(bot->getSide())) != 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
