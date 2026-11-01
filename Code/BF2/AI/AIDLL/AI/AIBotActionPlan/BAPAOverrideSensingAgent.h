// Tells the sensing agent to leave the view to the plan for a while. Retail reports the work as
// not done by an entry, so the interpreter takes the statement off the list at once.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAOVERRIDESENSINGAGENT_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAOVERRIDESENSINGAGENT_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAOverrideSensingAgent : public BAPAction
{
public:
    BAPAOverrideSensingAgent() : BAPAction(0) {}
    virtual ~BAPAOverrideSensingAgent();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPAOVERRIDESENSINGAGENT_H
