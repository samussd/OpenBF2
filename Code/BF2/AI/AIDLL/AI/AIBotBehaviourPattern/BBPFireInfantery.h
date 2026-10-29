// Plan generator for the behaviour of the same family. sizeof == 0x34.
// [FRONTIER] Plan creation and destruction remain named stops.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIREINFANTERY_H
#define AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIREINFANTERY_H

#include <list>

#include "BBPFire.h"

namespace dice {
namespace hfe {
namespace ai {

class BBPFireInfantery : public BBPFire
{
public:
    // Per-bot firing state, 0x38 bytes. [UNRESOLVED] field roles.
    struct FiringState
    {
        FiringState();
        /* +0x00 */ int m_int00;
        /* +0x04 */ int m_int04;
        /* +0x08 */ int m_int08;
        /* +0x0C */ bool m_flag0C;
        /* +0x10 */ int m_int10;           // not written by the constructor
        /* +0x14 */ bool m_flags14[4];
        /* +0x18 */ Vec3 m_vec18;
        /* +0x24 */ bool m_flags24[3];
        /* +0x28 */ Vec3 m_vec28;
        /* +0x34 */ float m_float34;
    };

    // Per-bot fire decisions, 0x5C bytes. [UNRESOLVED] field roles; a FireDecision is
    // four bytes.
    struct Data
    {
        struct FireDecision { int m_value; };
        Data();
        /* +0x00 */ int m_int00[2];        // not written by the constructor
        /* +0x08 */ int m_int08;
        /* +0x0C */ std::list<FireDecision> m_decisions;
        /* +0x10 */ int m_int10;
        /* +0x14 */ double m_double14;
        /* +0x1C */ double m_double1C;
        /* +0x24 */ Vec3 m_vec24;
        /* +0x30 */ Vec3 m_vec30;
        /* +0x3C */ Vec3 m_vec3C;
        /* +0x48 */ int m_int48;           // not written by the constructor
        /* +0x4C */ int m_int4C;
        /* +0x50 */ int m_int50;
        /* +0x54 */ int m_int54[2];        // not written by the constructor
    };

    explicit BBPFireInfantery(int behaviourId);
    virtual ~BBPFireInfantery();

    virtual BotScript* createPlan(Bot* bot);                         // +0x08
    virtual void init(Bot* bot, unsigned int target, int weapon, bool b, bool c);                 // +0x18
    virtual void init(Bot* bot, unsigned int target, const Vec3& position, int weapon);       // +0x1C
    virtual int getImplementationType();                            // +0x20

protected:
    /* +0x28 */ FiringState* m_firingStates;                 // per bot
    /* +0x2C */ Data* m_data;                                // per bot
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOURPATTERN_BBPFIREINFANTERY_H
