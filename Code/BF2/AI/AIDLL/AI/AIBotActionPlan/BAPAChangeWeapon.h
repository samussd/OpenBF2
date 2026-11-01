// Selects a weapon by pressing its key. A soldier stops firing and reloading first, and the
// statement lasts until the weapon is the active one. A vehicle presses the key once.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACHANGEWEAPON_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACHANGEWEAPON_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class WeaponInfo;

class BAPAChangeWeapon : public BAPAction
{
public:
    BAPAChangeWeapon(Bot* bot, int weapon, int planId);
    virtual ~BAPAChangeWeapon();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ Bot* m_bot;
    /* +0x18 */ int m_weapon;
    /* +0x1C */ WeaponInfo* m_weaponInfo;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACHANGEWEAPON_H
