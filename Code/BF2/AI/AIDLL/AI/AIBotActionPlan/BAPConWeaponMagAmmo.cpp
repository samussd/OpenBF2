#include "BAPConWeaponMagAmmo.h"

#include "../AIKnowledge/WeaponInfo.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x202c00 (Ghidra).
BAPConWeaponMagAmmo::BAPConWeaponMagAmmo(BAPVariable<WeaponInfo*>* weapon, BAPVariable<int>* ammo, bool above)
    : m_weapon(weapon)
    , m_above(above)
    , m_ammo(ammo)
{
}

BAPConWeaponMagAmmo::~BAPConWeaponMagAmmo()
{
}

// 0x202c80 (Ghidra). A negative count means the weapon does not report one.
bool BAPConWeaponMagAmmo::evaluate(Bot*)
{
    WeaponInfo* const weapon = m_weapon->m_value;
    const int mags = weapon->getNumberOfMags();
    if (m_above)
    {
        if (mags == 0)
            return m_ammo->m_value != 0;
        if (mags < 0)
            return false;
        return weapon->getShotsInMag(0) != m_ammo->m_value;
    }
    if (mags < 0)
        return false;
    if (mags == 0)
        return true;
    const int shots = weapon->getShotsInMag(0);
    return shots >= 0 && shots <= m_ammo->m_value;
}

} // namespace ai
} // namespace hfe
} // namespace dice
