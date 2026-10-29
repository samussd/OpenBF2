// Behaviour built by BBCreateFunctionFire; BBFireInfantery extends it. sizeof == 0x40.
// [FRONTIER] Destruction remains a named stop.

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
    // [UNRESOLVED] When the bot last fired at the target to some effect; the urgency checks it
    // against 20 s. Eight bytes, a double.
    struct BBPFeedback
    {
        BBPFeedback() : m_time(-10000.0) {}
        double m_time;
    };

    BBFire(unsigned char behaviourId, unsigned char mode, BBPFire* generator, UrgeCurve* urgeCurve,
           float* weights);
    virtual ~BBFire();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    virtual void initializeGenerator(BotMain* bot, int weapon, bool b, bool c);   // +0x34
    virtual void initializeGenerator(unsigned int target, const Vec3& position, BotMain* bot,
                                     int weapon);                    // +0x38

    // The target's record, made on first use.
    BBPFeedback* getBBPFeedback(unsigned int handle);
    // The index of the bot's best weapon on the target at that distance, or -1.
    int chooseWeapon(Bot* bot, unsigned int target, float distance);

protected:
    /* +0x24 */ int* m_weapons;                       // per bot, the chosen weapon index or -1
    /* +0x28 */ double m_lastPurge;                   // when m_feedback was last purged
    /* +0x30 */ std::map<unsigned int, BBPFeedback*> m_feedback;
    /* +0x3C */ CustomCurve* m_curve;                 // a soft step sampled every 0.03 on [0, 1]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBFIRE_H
