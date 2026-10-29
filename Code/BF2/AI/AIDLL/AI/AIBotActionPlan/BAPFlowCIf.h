// Runs its true or false statement by the condition. It owns the condition and both statements.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCIF_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCIF_H

#include "BAPFlowControl.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPFlowCIf : public BAPFlowControl
{
public:
    BAPFlowCIf(BAPCondition* condition, BAPStatement* trueStatement, BAPStatement* falseStatement,
               int planId, bool reevaluate, bool keepFalse);
    virtual ~BAPFlowCIf();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual void debug(BotScriptDebugger::DebugLevel level, std::string& text);   // [FRONTIER]
    virtual void streamSelf(std::ostream& out) const;                              // [FRONTIER]

    BAPCondition* getCondition();
    BAPStatement* getTrueStatement();
    BAPStatement* getFalseStatement();
    BAPStatement* getActiveStatement();
    BAPStatement* getActiveStatementDebug();

private:
    /* +0x14 */ BAPCondition* m_condition;
    /* +0x18 */ BAPStatement* m_trueStatement;
    /* +0x1C */ BAPStatement* m_falseStatement;
    /* +0x20 */ BAPStatement* m_activeStatement;     // cleared when it finishes
    /* +0x24 */ BAPStatement* m_lastActiveStatement; // kept for the debugger
    // Test the condition on every step, not only when nothing is running.
    /* +0x28 */ bool m_reevaluate;
    // With m_reevaluate, a running false statement is not interrupted.
    /* +0x29 */ bool m_keepFalse;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPFLOWCIF_H
