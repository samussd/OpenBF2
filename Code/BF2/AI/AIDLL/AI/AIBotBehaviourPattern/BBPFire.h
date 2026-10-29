// The fire family's base. sizeof == 0x28.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIRE_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIRE_H

#include "BBPPattern.h"

namespace dice {
namespace hfe {
namespace ai {

class BotBehaviour;

class BBPFire : public BBPPattern
{
public:
    BBPFire();
    virtual ~BBPFire();

    // Fire at the target with the weapon at that index; or at the target from the position.
    virtual void init(Bot* bot, unsigned int target, int weapon, bool b, bool c) = 0;            // +0x18
    virtual void init(Bot* bot, unsigned int target, const Vec3& position, int weapon) = 0;  // +0x1C
    virtual int getImplementationType();                            // +0x20

    // The behaviour whose urgency this generator reports to.
    void setUrgencyGenerator(BotBehaviour* behaviour);

protected:
    /* +0x08 */ int m_behaviourId;                           // the subclasses store it
    /* +0x0C */ int* m_targets;                              // per bot, the target handle
    /* +0x10 */ bool* m_changedTarget;                       // per bot
    /* +0x14 */ int* m_weapons;                              // per bot, the weapon index
    /* +0x18 */ int* m_positionTargets;                      // per bot, the second init's target
    /* +0x1C */ Vec3* m_positions;                           // per bot, zero; the second init's
    /* +0x20 */ int m_int20;                                 // [UNRESOLVED] not written by the constructor
    /* +0x24 */ BotBehaviour* m_urgencyGenerator;            // setUrgencyGenerator
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIRE_H
