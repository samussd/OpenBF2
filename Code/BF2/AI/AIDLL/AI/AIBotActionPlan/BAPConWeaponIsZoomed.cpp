#include "BAPConWeaponIsZoomed.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"

namespace dice {
namespace hfe {
namespace ai {

BAPConWeaponIsZoomed::~BAPConWeaponIsZoomed()
{
}

// Retail asks the armament for its active weapon twice and does not guard the equipment or its
// armament.
bool BAPConWeaponIsZoomed::evaluate(Bot* bot)
{
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    IPIArmament* const armament = equipment->getArmament();
    armament->getWeapon(armament->getActiveWeapon());
    WeaponInfo* const weapon =
        WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));
    return weapon ? weapon->isZoomed() : false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
