#include "BAPAChangeWeapon.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../../../../IO/Input/PlayerInput.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1e0e10 (Ghidra).
BAPAChangeWeapon::BAPAChangeWeapon(Bot* bot, int weapon, int planId)
    : BAPAction(planId)
    , m_bot(bot)
    , m_weapon(weapon)
{
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    m_weaponInfo = WeaponInfoManager::m_instance->getInformation(equipment->getArmament()->getWeapon(weapon));
    m_controls = 1ULL << m_weaponInfo->getWeaponActivate();
}

BAPAChangeWeapon::~BAPAChangeWeapon()
{
}

// 0x1e10c0 (Ghidra). With no equipment ok stays as the caller set it, false here.
bool BAPAChangeWeapon::execute(Bot* bot, bool& ok, bool& done)
{
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    io::PlayerInput* const input = static_cast<BotMain*>(bot)->getPlayerInput();
    if (!equipment)
    {
        ok = false;
    }
    else
    {
        const io::PlayerInputMap key = static_cast<io::PlayerInputMap>(m_weaponInfo->getWeaponActivate());
        if ((equipment->getObjectTypeFlags() & 0x200000u) != 0)
        {
            const bool active = equipment->getArmament()->getActiveWeapon() == m_weapon;
            if (active)
                input->setInput(key, 0.0f);
            input->setInput(io::PIFire, 0.0f);
            input->setInput(io::PIAltFire, 0.0f);
            input->setInput(io::PIReload, 0.0f);
            if (!active)
                input->setInput(key, 1.0f);
            ok = true;
            done = active;
            return true;
        }
        if (static_cast<int>(key) < 64 && ((input->mappingFlags >> key) & 1ULL))
            input->inputs[key] = 1.0f;
        ok = true;
    }
    done = true;
    return true;
}

BAPStatementType BAPAChangeWeapon::getId() const
{
    return BAPSTChangeWeapon;
}

bool BAPAChangeWeapon::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
