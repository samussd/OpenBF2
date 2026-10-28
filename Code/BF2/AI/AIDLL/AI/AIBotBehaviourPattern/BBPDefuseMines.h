// Plan generator for the behaviour of the same family. sizeof == 0x1C.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPDEFUSEMINES_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPDEFUSEMINES_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPDefuseMines : public BBPPattern
{
public:
    explicit BBPDefuseMines(int behaviourId);
    virtual ~BBPDefuseMines();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08

protected:
    /* +0x08 */ int m_behaviourId;
    /* +0x0C */ int* m_int0C;                                // [UNRESOLVED] per bot, -1
    /* +0x10 */ int* m_int10;                                // [UNRESOLVED] per bot, -1
    /* +0x14 */ Vec3* m_vec14;                               // [UNRESOLVED] per bot, zero
    /* +0x18 */ Vec3* m_vec18;                               // [UNRESOLVED] per bot, zero
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPDEFUSEMINES_H
