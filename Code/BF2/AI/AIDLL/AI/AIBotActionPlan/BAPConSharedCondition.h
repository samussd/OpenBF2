// Keeps a condition's last result where several statements can read it: only the first holder
// evaluates the condition, and the copies from getNewSharedCondition read what it found.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONSHAREDCONDITION_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONSHAREDCONDITION_H

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConSharedCondition : public BAPCondition
{
public:
    explicit BAPConSharedCondition(BAPCondition* condition);
    explicit BAPConSharedCondition(BAPConSharedCondition& other);
    virtual ~BAPConSharedCondition();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 38; }

    BAPConSharedCondition* getNewSharedCondition();

private:
    struct Shared
    {
        /* +0x00 */ bool m_result;
        /* +0x04 */ int m_holders;
    };

    /* +0x08 */ Shared* m_shared;
    /* +0x0C */ BAPCondition* m_condition;      // null in the copies
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONSHAREDCONDITION_H
