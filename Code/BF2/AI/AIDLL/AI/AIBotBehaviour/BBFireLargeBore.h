// Behaviour built by BBCreateFunctionFireLargeBore. Not a BBFire, though it opens the same way.
// sizeof == 0x40.
// [FRONTIER] Urgency, generator set-up and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRELARGEBORE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRELARGEBORE_H

#include <map>

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPFire;
class BotMain;

class BBFireLargeBore : public BotBehaviour
{
public:
    BBFireLargeBore(unsigned char behaviourId, unsigned char mode, BBPFire* generator, UrgeCurve* urgeCurve,
                    float* weights, bool flag);
    virtual ~BBFireLargeBore();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    virtual void initializeGenerator(BotMain* bot, int a, bool b, bool c, bool d);   // +0x34

protected:
    /* +0x24 */ int* m_int24;                         // [UNRESOLVED] per bot, -1
    /* +0x28 */ double m_double28;                    // [UNRESOLVED] zero
    // [UNRESOLVED] key and value types: the ELF's nodes carry eight bytes of value.
    /* +0x30 */ std::map<int, int> m_map30;
    /* +0x3C */ bool m_flag3C;                        // [UNRESOLVED] the factory passes false
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRELARGEBORE_H
