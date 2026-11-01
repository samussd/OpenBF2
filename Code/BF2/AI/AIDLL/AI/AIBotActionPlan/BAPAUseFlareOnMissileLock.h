// Answers a missile lock with the vehicle's flares, after a delay for vehicles that can dodge.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAUSEFLAREONMISSILELOCK_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAUSEFLAREONMISSILELOCK_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAUseFlareOnMissileLock : public BAPAction
{
public:
    BAPAUseFlareOnMissileLock(Bot* bot, unsigned long long controls, int planId);
    virtual ~BAPAUseFlareOnMissileLock();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ Bot* m_bot;
    /* +0x18 */ double m_lockTime;
    /* +0x20 */ bool m_locked;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAUSEFLAREONMISSILELOCK_H
