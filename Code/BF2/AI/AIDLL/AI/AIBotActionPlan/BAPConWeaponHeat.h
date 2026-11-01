// Lets a heating weapon fire until the heat reaches the high mark, then waits for it to fall to
// the low mark.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONHEAT_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONHEAT_H

#include "BAPCondition.h"
#include "../AIPlanInterpreter/BAPVariable.h"

namespace dice {
namespace hfe {
namespace ai {

class WeaponInfo;

class BAPConWeaponHeat : public BAPCondition
{
public:
    BAPConWeaponHeat(BAPVariable<WeaponInfo*>* weapon, float heat);
    BAPConWeaponHeat(BAPVariable<WeaponInfo*>* weapon, float high, float low);
    virtual ~BAPConWeaponHeat();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 46; }

private:
    /* +0x08 */ BAPVariable<WeaponInfo*>* m_weapon;
    /* +0x0C */ float m_high;
    /* +0x10 */ float m_low;
    /* +0x14 */ bool m_cooling;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONWEAPONHEAT_H
