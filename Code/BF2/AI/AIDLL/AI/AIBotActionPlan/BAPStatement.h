// A plan statement: the controls it drives, its statement type and the plan generator that built it.
// news and destroys count every statement made and destroyed. AIBotActionPlan/BAPAction.h holds the
// base of the single actions.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPSTATEMENT_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPSTATEMENT_H

#include <iosfwd>
#include <string>

#include "BotScript.h"
#include "../AIPlanInterpreter/BAPStatementType.h"
#include "../AIPlanInterpreter/BotScriptDebugger.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPStatement : public BotScript
{
public:
    explicit BAPStatement(int planId);
    virtual ~BAPStatement();                                                       // +0x04 / +0x08

    // The player inputs the statement drives, one bit per io::PlayerInputMap entry.
    virtual unsigned long long getControls();                                      // +0x0C
    virtual BAPStatementType getId() const = 0;                                    // +0x10
    // Whether the statement executes itself where no interpreter entry is registered.
    virtual bool hasDefault() = 0;                                                 // +0x14
    virtual void debug(BotScriptDebugger::DebugLevel level, std::string& text);    // +0x18 [FRONTIER]
    virtual void streamSelf(std::ostream& out) const;                              // +0x1C [FRONTIER]

    static int news;
    static int destroys;

    /* +0x08 */ unsigned long long m_controls;
    // The id of the plan generator that built the statement; createPlan compares it to reuse a plan.
    /* +0x10 */ int m_planId;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPSTATEMENT_H
