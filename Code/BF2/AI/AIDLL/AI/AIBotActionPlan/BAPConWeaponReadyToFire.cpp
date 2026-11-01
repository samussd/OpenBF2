#include "BAPConWeaponReadyToFire.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x202df0 (Ghidra).
BAPConWeaponReadyToFire::BAPConWeaponReadyToFire(BAPVariable<WeaponInfo*>* weapon)
    : m_weapon(weapon)
{
}

BAPConWeaponReadyToFire::~BAPConWeaponReadyToFire()
{
}

// 0x202ed0 (Ghidra).
bool BAPConWeaponReadyToFire::evaluate(Bot* bot)
{
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    IPIArmament* const armament = equipment->getArmament();
    WeaponInfo* const active =
        WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));
    return m_weapon->m_value == active && active->isReadyToFire();
}

} // namespace ai
} // namespace hfe
} // namespace dice
