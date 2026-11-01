// True while the weapon in a variable is the active one and has something to detonate.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONDETONATORISACTIVE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONDETONATORISACTIVE_H

#include "BAPCondition.h"
#include "../AIPlanInterpreter/BAPVariable.h"

namespace dice {
namespace hfe {
namespace ai {

class WeaponInfo;

class BAPConWeaponDetonatorIsActive : public BAPCondition
{
public:
    explicit BAPConWeaponDetonatorIsActive(BAPVariable<WeaponInfo*>* weapon);
    virtual ~BAPConWeaponDetonatorIsActive();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 53; }

private:
    /* +0x08 */ BAPVariable<WeaponInfo*>* m_weapon;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONDETONATORISACTIVE_H
