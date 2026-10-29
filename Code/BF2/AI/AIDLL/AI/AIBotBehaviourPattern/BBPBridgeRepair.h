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
    // Marks the bot initiated on the segment: the tool's weapon index, the bridge direction,
    // whether a squad request is near it, where to stand and where to look.
    void init(unsigned int target, int toolIndex, Vec3 direction, Bot* bot, bool nearRequest,
              Vec3 stand, Vec3 look);

protected:
    /* +0x08 */ int m_behaviourId;
    // per bot, as init stores them
    /* +0x0C */ int* m_int0C;                                // -1; the segment's handle
    /* +0x10 */ int* m_int10;                                // -1; the tool's weapon index
    /* +0x14 */ Vec3* m_vec14;                               // zero; the bridge direction
    /* +0x18 */ bool* m_bool18;                              // false; a squad request near it
    /* +0x1C */ Vec3* m_vec1C;                               // zero; where to stand
    /* +0x20 */ Vec3* m_vec20;                               // zero; where to look
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPBRIDGEREPAIR_H
