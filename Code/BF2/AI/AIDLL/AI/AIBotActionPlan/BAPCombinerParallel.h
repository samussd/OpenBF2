// Runs every statement of its list on each step. The constructors that take a list are not recovered.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINERPARALLEL_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINERPARALLEL_H

#include "BAPCombiner.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPCombinerParallel : public BAPCombiner
{
public:
    BAPCombinerParallel(int planId, int n, bool neverFinish);                    // waits for all
    BAPCombinerParallel(int planId, bool waitForAll, int n, bool neverFinish);
    virtual ~BAPCombinerParallel();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual unsigned long long getControls();
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();
    virtual void streamSelf(std::ostream& out) const;                             // [FRONTIER]
    // Takes ownership; the statement's controls join the combination's.
    virtual void addStatement(BAPStatement* statement);

    std::list<bool>& getIsFinished();

private:
    // One flag per statement. [UNRESOLVED] execute only ever writes the first one; see the .cpp.
    /* +0x20 */ std::list<bool> m_isFinished;
    /* +0x24 */ int m_int24;              // [UNRESOLVED] 99 in the idle plan; nothing here reads it
    /* +0x28 */ bool m_neverFinish;       // execute clears done when set
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCOMBINERPARALLEL_H
