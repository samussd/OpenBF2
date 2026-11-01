
#include "IPIArmamentKnown.h"

#include "WeaponInfo.h"
#include "WeaponInfoManager.h"

namespace dice {
namespace hfe {
namespace ai {

IPIArmamentKnown::IPIArmamentKnown(Information* information)
    : IPIArmament(information)
    , m_stale(false)
    , m_activeWeapon(-1)
{
    updateWeapons();
    updateWeaponsInternal();
}

IPIArmamentKnown::~IPIArmamentKnown()
{
    for (std::vector<unsigned int>::iterator it = m_weapons.begin(); it != m_weapons.end(); ++it)
        WeaponInfoManager::m_instance->freeInformation(*it);
}

// Every remembered weapon refreshes itself.
void IPIArmamentKnown::setTime(double /*time*/)
{
    if (m_stale)
        updateWeaponsInternal();
    for (unsigned int i = 0; i < m_weapons.size(); ++i)
    {
        if (m_weapons[i] == 0xFFFFFFFFu)
            continue;
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(m_weapons[i]);
        if (weapon)
            weapon->update();
    }
}

int IPIArmamentKnown::getActiveWeapon() const
{
    if (m_stale)
        const_cast<IPIArmamentKnown*>(this)->updateWeaponsInternal();
    return m_activeWeapon;
}

unsigned int IPIArmamentKnown::getWeapon(int index)
{
    if (m_stale)
        updateWeaponsInternal();
    return m_weapons[index];
}

std::vector<unsigned int>& IPIArmamentKnown::getWeapons()
{
    if (m_stale)
        updateWeaponsInternal();
    return m_weapons;
}

void IPIArmamentKnown::updateWeapons()
{
    m_stale = true;
}

void IPIArmamentKnown::updateWeaponsInternal()
{
    IPIArmament::updateWeaponsInternal(false);
    m_stale = false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
