// Behaviour built by BBCreateFunctionFire; BBFireInfantery extends it. sizeof == 0x40.
// [FRONTIER] Urgency, generator set-up and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRE_H

#include <map>

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPFire;
class BotMain;
class CustomCurve;

class BBFire : public BotBehaviour
{
public:
    BBFire(unsigned char behaviourId, unsigned char mode, BBPFire* generator, UrgeCurve* urgeCurve,
           float* weights);
    virtual ~BBFire();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    virtual void initializeGenerator(BotMain* bot, int a, bool b, bool c);   // +0x34
    virtual void initializeGenerator(unsigned int target, const Vec3& position, BotMain* bot,
                                     int a);                         // +0x38

protected:
    /* +0x24 */ int* m_int24;                         // [UNRESOLVED] per bot, -1
    /* +0x28 */ double m_double28;                    // [UNRESOLVED] zero
    // [UNRESOLVED] key and value types: the ELF's nodes carry eight bytes of value.
    /* +0x30 */ std::map<int, int> m_map30;
    /* +0x3C */ CustomCurve* m_curve;                 // a soft step sampled every 0.03 on [0, 1]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRE_H
