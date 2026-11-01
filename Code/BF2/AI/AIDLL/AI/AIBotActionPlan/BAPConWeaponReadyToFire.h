// True while the weapon in a variable is the active one and can fire.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONREADYTOFIRE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONREADYTOFIRE_H

#include "BAPCondition.h"
#include "../AIPlanInterpreter/BAPVariable.h"

namespace dice {
namespace hfe {
namespace ai {

class WeaponInfo;

class BAPConWeaponReadyToFire : public BAPCondition
{
public:
    explicit BAPConWeaponReadyToFire(BAPVariable<WeaponInfo*>* weapon);
    virtual ~BAPConWeaponReadyToFire();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 44; }

private:
    /* +0x08 */ BAPVariable<WeaponInfo*>* m_weapon;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONREADYTOFIRE_H
