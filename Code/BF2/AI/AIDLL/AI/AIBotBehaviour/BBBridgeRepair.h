// Behaviour built by BBCreateFunctionBridgeRepair. sizeof == 0x3C.
// [FRONTIER] Urgency and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBBRIDGEREPAIR_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBBRIDGEREPAIR_H

#include "BotBehaviour.h"

namespace dice {
namespace hfe {
namespace ai {

class PlanGenerator;

class BBBridgeRepair : public BotBehaviour
{
public:
    BBBridgeRepair(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBBridgeRepair();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // A new slot, not an override: the spelling differs from BotBehaviour::isAplicable.
    virtual bool isApplicable(Information* information, Bot* bot);   // +0x34

    // Where to stand to repair the segment: out from its position along the bridge direction
    // (or against it) by the segment's radius and 1.5 up, then up to ten more steps until the
    // squad allows the position, on the side nearer the bot. look gets the unstepped point of
    // that side. False when neither side has a position.
    bool getBridgeCoordinate(unsigned int handle, Vec3 direction, Bot* bot, Vec3 botPosition,
                             Vec3& stand, Vec3& look);

protected:
    /* +0x24 */ int m_words24[3];                        // [UNRESOLVED] three zero words
    /* +0x30 */ int* m_int30;                            // [UNRESOLVED] one per battle strength
    /* +0x34 */ int* m_int34;                            // [UNRESOLVED] one per battle strength
    /* +0x38 */ int* m_int38;                            // [UNRESOLVED] one per battle strength
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBBRIDGEREPAIR_H
