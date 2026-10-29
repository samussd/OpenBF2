// A statement that runs a list of statements it owns. With m_waitForAll the combination finishes when
// every statement has finished, otherwise when any has.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINER_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINER_H

#include <list>

#include "BAPStatement.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCombiner : public BAPStatement
{
public:
    BAPCombiner(int planId, bool waitForAll);
    virtual ~BAPCombiner();
    virtual unsigned long long getControls() = 0;                 // +0x0C, pure again here
    virtual void addStatement(BAPStatement* statement) = 0;       // +0x20

protected:
    /* +0x14 */ std::list<BAPStatement*> m_statements;
    /* +0x18 */ std::list<BAPStatement*>::iterator m_current;    // the statement being run
    /* +0x1C */ bool m_waitForAll;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINER_H
