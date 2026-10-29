// Behaviour built by BBCreateFunctionMedicAssist. sizeof == 0x3C.
// [FRONTIER] Destruction remains a named stop.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMEDICASSIST_H
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMEDICASSIST_H

#include "BotBehaviour.h"

#include <vector>

namespace dice {
namespace hfe {
namespace ai {

class CommunicationAgent;
class PlanGenerator;

class BBMedicAssist : public BotBehaviour
{
public:
    BBMedicAssist(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights);
    virtual ~BBMedicAssist();

    virtual float calculateUrgency(Bot* bot, bool flag, float value);   // +0x28
    // A bot that is not down and carries a weapon of type 1 or 2.
    virtual bool isAplicable(Information* information, Bot* bot);   // +0x30

    // Whether a human occupies the primary object of the information, for the side, or one of
    // its secondary objects.
    bool isHumanInTarget(Information* information, int side);
    // Whether an engineer request (radio message 4) came from the player behind the information
    // or behind one of its secondary objects.
    bool vehicleRequestsEngineer(Information* information, int side, CommunicationAgent* agent) const;
    // The support soldier's urgency to hand out ammunition.
    float calculateGiveAmmoUrgency(Bot* bot, float value);

protected:
    /* +0x24 */ std::vector<unsigned int> m_candidates;  // the units of the last evaluation
    // Per battle strength, the bot's best tool against it: [INFERRED] names.
    /* +0x30 */ int* m_toolIndex;                        // index in the armament, -1 for none
    /* +0x34 */ float* m_toolRange;                      // its maximum range
    /* +0x38 */ int* m_toolStrength;                     // its strength
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_BBMEDICASSIST_H
