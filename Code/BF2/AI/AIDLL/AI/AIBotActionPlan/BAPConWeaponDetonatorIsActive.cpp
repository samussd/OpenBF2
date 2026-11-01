#include "BAPConWeaponDetonatorIsActive.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x2020b0 (Ghidra).
BAPConWeaponDetonatorIsActive::BAPConWeaponDetonatorIsActive(BAPVariable<WeaponInfo*>* weapon)
    : m_weapon(weapon)
{
}

BAPConWeaponDetonatorIsActive::~BAPConWeaponDetonatorIsActive()
{
}

// 0x202190 (Ghidra).
bool BAPConWeaponDetonatorIsActive::evaluate(Bot* bot)
{
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    IPIArmament* const armament = equipment->getArmament();
    WeaponInfo* const active =
        WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));
    return m_weapon->m_value == active && active->getWeaponState() == 1;
}

} // namespace ai
} // namespace hfe
} // namespace dice
