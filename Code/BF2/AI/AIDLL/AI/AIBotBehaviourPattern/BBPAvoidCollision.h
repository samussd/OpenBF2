// The avoid-collision family's base. sizeof == 0x1C.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPAvoidCollision : public BBPPattern
{
public:
    BBPAvoidCollision();
    virtual ~BBPAvoidCollision();

    virtual void init(Bot* bot, Vec3& a, unsigned int handle, Vec3& b, float f) = 0;   // +0x18

protected:
    /* +0x08 */ int m_behaviourId;                           // the subclasses store it
    /* +0x0C */ int* m_int0C;                                // [UNRESOLVED] per bot, not initialised
    /* +0x10 */ Vec3* m_vec10;                               // [UNRESOLVED] per bot, zero
    /* +0x14 */ int* m_int14;                                // [UNRESOLVED] per bot, not initialised
    /* +0x18 */ int m_int18;                                 // [UNRESOLVED] not written by the constructor
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPAVOIDCOLLISION_H
