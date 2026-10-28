// The idle family's base. sizeof == 0xC.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPIdle : public BBPPattern
{
public:
    explicit BBPIdle(int behaviourId);
    virtual ~BBPIdle();

    virtual int getType();                                          // +0x18

protected:
    /* +0x08 */ int m_behaviourId;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPIDLE_H
