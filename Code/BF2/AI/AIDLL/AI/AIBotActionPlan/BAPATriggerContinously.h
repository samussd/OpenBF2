// Holds one player input while the run condition holds, until a break condition. The work is the
// interpreter entry's (EntryTriggerContinously); the statement alone does not run. Dice's spelling.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPATRIGGERCONTINOUSLY_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPATRIGGERCONTINOUSLY_H

#include "BAPAction.h"
#include "../../../../IO/Input/Input.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCondition;

class BAPATriggerContinously : public BAPAction
{
public:
    BAPATriggerContinously(io::PlayerInputMap input, BAPCondition* runCondition, BAPCondition* breakCondition,
                           BAPCondition* third, unsigned long long controls, int planId, double delay);
    virtual ~BAPATriggerContinously();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

    /* +0x14 */ io::PlayerInputMap m_input;
    /* +0x18 */ BAPCondition* m_runCondition;
    /* +0x1C */ BAPCondition* m_breakCondition;
    /* +0x20 */ BAPCondition* m_third;      // [UNRESOLVED] name
    /* +0x24 */ bool m_pressed;             // [UNRESOLVED] = false
    /* +0x28 */ double m_startTime;         // the game time at construction plus the delay
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPATRIGGERCONTINOUSLY_H
