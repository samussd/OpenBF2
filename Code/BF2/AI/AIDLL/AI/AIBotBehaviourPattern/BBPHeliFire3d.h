// Plan generator for the behaviour of the same family. sizeof == 0x28.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPHELIFIRE3D_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPHELIFIRE3D_H

#include "BBPFire.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPHeliFire3d : public BBPFire
{
public:
    explicit BBPHeliFire3d(int behaviourId);
    virtual ~BBPHeliFire3d();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual void init(Bot* bot, unsigned int target, int a, bool b, bool c);                 // +0x18
    virtual void init(Bot* bot, unsigned int target, const Vec3& position, int a);       // +0x1C
    virtual int getImplementationType();                            // +0x20
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPHELIFIRE3D_H
