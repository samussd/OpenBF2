// Makes the bot forget an object it sensed.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAFORGETTARGET_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAFORGETTARGET_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAForgetTarget : public BAPAction
{
public:
    explicit BAPAForgetTarget(unsigned int target);
    virtual ~BAPAForgetTarget();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ unsigned int m_target;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAFORGETTARGET_H
