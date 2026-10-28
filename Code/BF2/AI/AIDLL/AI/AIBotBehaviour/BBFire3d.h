// Behaviour built by BBCreateFunctionFire3d. Not a BBFire, though it opens the same way.
// sizeof == 0x3C.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRE3D_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRE3D_H

#include <map>

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPFire;

class BBFire3d : public BotBehaviour
{
public:
    BBFire3d(unsigned char behaviourId, unsigned char mode, BBPFire* generator, UrgeCurve* urgeCurve,
             float* weights);
    virtual ~BBFire3d();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28

protected:
    /* +0x24 */ int* m_int24;                         // [UNRESOLVED] per bot, -1
    /* +0x28 */ double m_double28;                    // [UNRESOLVED] zero
    // [UNRESOLVED] key and value types: the ELF's nodes carry eight bytes of value.
    /* +0x30 */ std::map<int, int> m_map30;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRE3D_H
