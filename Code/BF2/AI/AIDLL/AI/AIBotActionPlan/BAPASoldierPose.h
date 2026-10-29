// Takes a soldier pose. The work is the interpreter entry's (EntrySoldierPose); the statement alone
// does not run. Only the (pose, run condition, plan id, bool) constructor is recovered.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASOLDIERPOSE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASOLDIERPOSE_H

#include "BAPAction.h"
#include "BAPAComponentSoldierPose.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPASoldierPose : public BAPAction, public BAPAComponentSoldierPose
{
public:
    BAPASoldierPose(SoldierPose pose, BAPCondition* runCondition, int planId, bool b);
    virtual ~BAPASoldierPose();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

    BAPCondition* getRunCondition();
    BAPCondition* getBreakCondition();

    /* +0x40 */ BAPCondition* m_runCondition;
    /* +0x44 */ BAPCondition* m_breakCondition;
    /* +0x48 */ bool m_bool48;     // [UNRESOLVED] passed on to Bot::requestSoldierPose
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASOLDIERPOSE_H
