// Runs the statements of its list one after another, one statement per step. Waiting for each, a
// statement runs until it is done and the combination is done after the last; otherwise it moves
// on every step and is done when a statement is, or after the last. Done, it starts over.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINERSERIAL_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINERSERIAL_H

#include "BAPCombiner.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCombinerSerial : public BAPCombiner
{
public:
    BAPCombinerSerial(int planId, int n);                                         // waits for each
    BAPCombinerSerial(int planId, bool waitForAll, int n);
    // Takes the statements over; their controls are not added.
    BAPCombinerSerial(std::list<BAPStatement*>& statements, int planId, int n);
    virtual ~BAPCombinerSerial();
    virtual bool execute(Bot* bot, bool& ok, bool& done);
    // The controls of the statement being run.
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual void streamSelf(std::ostream& out) const;                             // [FRONTIER]
    // Takes ownership and starts over from the first statement. The controls are not added.
    virtual void addStatement(BAPStatement* statement);

private:
    // [UNRESOLVED] -1 in the move plans; the debugger prints it when set. Nothing here reads it.
    /* +0x20 */ int m_int20;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINERSERIAL_H
