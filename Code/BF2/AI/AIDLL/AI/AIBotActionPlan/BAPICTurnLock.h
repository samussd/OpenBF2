// Sets the bot's plan lock, which keeps the current plan from being replaced.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPICTURNLOCK_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPICTURNLOCK_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPICTurnLock : public BAPAction
{
public:
    explicit BAPICTurnLock(bool locked);
    virtual ~BAPICTurnLock();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ bool m_locked;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPICTURNLOCK_H
