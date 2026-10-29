// Releases the bot's controls. The work is the interpreter entry's (EntryInfanteryResetControls and
// the vehicle ones). Only the (movement, look, actions, plan id) constructor is recovered.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPARESETCONTROLS_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPARESETCONTROLS_H

#include "BAPAction.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPAResetControls : public BAPAction
{
public:
    BAPAResetControls(bool movement, bool look, bool actions, int planId);
    virtual ~BAPAResetControls();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual void streamSelf(std::ostream& out) const;     // [FRONTIER]

    // Whether the statement releases one input (its controls) rather than whole groups.
    bool hasSingleInput();
    int getInput();

    // [INFERRED] names, from infanteryResetControls' parameters.
    /* +0x14 */ bool m_movement;
    /* +0x15 */ bool m_look;
    /* +0x16 */ bool m_actions;
    /* +0x18 */ int m_input;           // the single input; 0 here
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPARESETCONTROLS_H
