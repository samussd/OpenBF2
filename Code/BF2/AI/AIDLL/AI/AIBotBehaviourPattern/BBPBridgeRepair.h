// Plan generator for the behaviour of the same family. sizeof == 0x24.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPBRIDGEREPAIR_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPBRIDGEREPAIR_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPBridgeRepair : public BBPPattern
{
public:
    explicit BBPBridgeRepair(int behaviourId);
    virtual ~BBPBridgeRepair();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08

protected:
    /* +0x08 */ int m_behaviourId;
    /* +0x0C */ int* m_int0C;                                // [UNRESOLVED] per bot, -1
    /* +0x10 */ int* m_int10;                                // [UNRESOLVED] per bot, -1
    /* +0x14 */ Vec3* m_vec14;                               // [UNRESOLVED] per bot, zero
    /* +0x18 */ bool* m_bool18;                              // [UNRESOLVED] per bot, false
    /* +0x1C */ Vec3* m_vec1C;                               // [UNRESOLVED] per bot, zero
    /* +0x20 */ Vec3* m_vec20;                               // [UNRESOLVED] per bot, zero
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPBRIDGEREPAIR_H
