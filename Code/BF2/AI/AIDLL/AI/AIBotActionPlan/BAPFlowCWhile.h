// Repeats its statement while the condition holds. The condition is tested again only once the
// statement has finished a pass. It owns both.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCWHILE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCWHILE_H

#include "BAPFlowControl.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPFlowCWhile : public BAPFlowControl
{
public:
    BAPFlowCWhile(BAPCondition* condition, BAPStatement* loopStatement, int planId);
    virtual ~BAPFlowCWhile();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual void streamSelf(std::ostream& out) const;     // [FRONTIER]

    void breakLoop();
    BAPCondition* getCondition();
    BAPStatement* getLoopStatement();

private:
    /* +0x14 */ BAPCondition* m_condition;
    /* +0x18 */ BAPStatement* m_loopStatement;
    /* +0x1C */ bool m_break;                 // breakLoop: the next step finishes the loop
    /* +0x1D */ bool m_testCondition;         // = true; test before the next step
    /* +0x1E */ bool m_testAfterPass;         // = true; test only after a finished pass
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCWHILE_H
