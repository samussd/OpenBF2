// True once a delay has passed. Each time it comes true the next test starts the delay again.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONTIME_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONTIME_H

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPConTime : public BAPCondition
{
public:
    // [UNRESOLVED] b is kept but never read. started takes the delay as already running, and
    // expired makes the first test true.
    BAPConTime(double delay, bool b, bool started, bool expired);
    virtual ~BAPConTime();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 39; }

private:
    /* +0x08 */ double m_end;
    /* +0x10 */ double m_delay;
    /* +0x18 */ bool m_started;
    /* +0x19 */ bool m_bool19;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONTIME_H
