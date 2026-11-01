// True while the weapon in a variable has locked on to something the bot's side knows about.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONHASLOCKONTARGET_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONHASLOCKONTARGET_H

#include "BAPCondition.h"
#include "../AIPlanInterpreter/BAPVariable.h"

namespace dice {
namespace hfe {
namespace ai {

class WeaponInfo;

class BAPConWeaponHasLockOnTarget : public BAPCondition
{
public:
    BAPConWeaponHasLockOnTarget(BAPVariable<WeaponInfo*>* weapon, unsigned int target);
    virtual ~BAPConWeaponHasLockOnTarget();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 47; }

private:
    /* +0x08 */ BAPVariable<WeaponInfo*>* m_weapon;
    /* +0x0C */ unsigned int m_target;      // not read: any known lock counts
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONHASLOCKONTARGET_H
