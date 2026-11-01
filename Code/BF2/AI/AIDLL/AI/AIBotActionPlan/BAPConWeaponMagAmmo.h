// Tests the shots left in the magazine of the weapon in a variable. With above it is true while
// the magazine does not hold exactly that many; otherwise while it holds that many or fewer.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONMAGAMMO_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONMAGAMMO_H

#include "BAPCondition.h"
#include "../AIPlanInterpreter/BAPVariable.h"

namespace dice {
namespace hfe {
namespace ai {

class WeaponInfo;

class BAPConWeaponMagAmmo : public BAPCondition
{
public:
    BAPConWeaponMagAmmo(BAPVariable<WeaponInfo*>* weapon, BAPVariable<int>* ammo, bool above);
    virtual ~BAPConWeaponMagAmmo();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 41; }

private:
    /* +0x08 */ BAPVariable<WeaponInfo*>* m_weapon;
    /* +0x0C */ bool m_above;
    /* +0x10 */ BAPVariable<int>* m_ammo;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONMAGAMMO_H
