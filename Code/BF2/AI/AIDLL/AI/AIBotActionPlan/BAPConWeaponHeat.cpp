#include "BAPConWeaponHeat.h"

#include "../AIKnowledge/WeaponInfo.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x202560 (Ghidra).
BAPConWeaponHeat::BAPConWeaponHeat(BAPVariable<WeaponInfo*>* weapon, float heat)
    : m_weapon(weapon)
    , m_high(heat)
    , m_low(heat)
    , m_cooling(false)
{
}

// 0x202620 (Ghidra).
BAPConWeaponHeat::BAPConWeaponHeat(BAPVariable<WeaponInfo*>* weapon, float high, float low)
    : m_weapon(weapon)
    , m_high(high)
    , m_low(low)
    , m_cooling(false)
{
}

BAPConWeaponHeat::~BAPConWeaponHeat()
{
}

// 0x2026e0 (Ghidra).
bool BAPConWeaponHeat::evaluate(Bot*)
{
    const float heat = m_weapon->m_value->getHeat();
    if (!m_cooling)
    {
        if (m_high <= heat)
        {
            m_cooling = true;
            return false;
        }
        return true;
    }
    if (m_low < heat)
        return false;
    m_cooling = false;
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
