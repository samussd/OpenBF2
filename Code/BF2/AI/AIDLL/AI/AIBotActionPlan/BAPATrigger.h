// Presses one player input while the run condition holds, until the break condition. The work is
// the interpreter entry's (EntryTrigger); the statement alone does not run.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPATRIGGER_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPATRIGGER_H

#include "BAPAction.h"
#include "../../../../IO/Input/Input.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPATrigger : public BAPAction
{
public:
    BAPATrigger(io::PlayerInputMap input, BAPCondition* runCondition, BAPCondition* breakCondition,
                unsigned long long controls, int planId, double delay, bool b);
    virtual ~BAPATrigger();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual void debug(BotScriptDebugger::DebugLevel level, std::string& text);   // [FRONTIER]
    virtual void streamSelf(std::ostream& out) const;                              // [FRONTIER]

    /* +0x14 */ io::PlayerInputMap m_input;
    /* +0x18 */ BAPCondition* m_runCondition;
    /* +0x1C */ BAPCondition* m_breakCondition;
    /* +0x20 */ bool m_bool20;          // [UNRESOLVED] = false
    /* +0x24 */ double m_startTime;     // the game time at construction plus the delay
    /* +0x2C */ bool m_bool2C;          // [UNRESOLVED] the last constructor argument
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPATRIGGER_H
