// Plan generator for the behaviour of the same family. sizeof == 0x2C.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIRE2D_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIRE2D_H

#include "BBPFire.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPFire2d : public BBPFire
{
public:
    explicit BBPFire2d(int behaviourId);
    virtual ~BBPFire2d();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual void init(Bot* bot, unsigned int target, int weapon, bool b, bool c);                 // +0x18
    virtual void init(Bot* bot, unsigned int target, const Vec3& position, int weapon);       // +0x1C
    virtual int getImplementationType();                            // +0x20

protected:
    /* +0x28 */ bool* m_bool28;                              // [UNRESOLVED] per bot, not initialised
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIRE2D_H
