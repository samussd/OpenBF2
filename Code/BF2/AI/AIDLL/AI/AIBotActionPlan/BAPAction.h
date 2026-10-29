// The base of the single actions of a plan (press a key, take a pose, release the controls). Actions
// with hasDefault false run only through an interpreter entry.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACTION_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACTION_H

#include "BAPStatement.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAction : public BAPStatement
{
public:
    explicit BAPAction(int planId) : BAPStatement(planId) {}
    virtual ~BAPAction();
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPACTION_H
