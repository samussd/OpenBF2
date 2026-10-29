// One thing a bot has sensed: the object's handle, how it came in and the times BotSenses keeps
// on it. BotSenses owns these, keyed by handle (operator new(0x68) in event_heardObject).

#ifndef AI_AIDLL_AI_AIBOT_SENSEDOBJECT_H
#define AI_AIDLL_AI_AIBOT_SENSEDOBJECT_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class SensedObject
{
public:
    // [INFERRED] parameter names: event_heardObject passes (handle, false, info->getSide() != 0).
    SensedObject(unsigned int handle, bool seen, bool sided);
    ~SensedObject();
    // An empty eight-step loop in the binary; it initialises nothing.
    void initStrengths();

    // How the object was sensed, one bit each. The constructor sets 2 for a seen object and
    // 1 | 8 | 0x10 otherwise, plus 4 when it has a side; SensingAgent::evaluateSensedObjects also
    // tests 0x20. [UNRESOLVED] bit names.
    enum
    {
        kFlag1  = 0x01,
        kSeen   = 0x02,
        kSided  = 0x04,
        kFlag8  = 0x08,
        kFlag10 = 0x10,
        kFlag20 = 0x20,
        kFlag40 = 0x40     // BBFire::chooseWeapon
    };

    /* +0x00 */ unsigned int m_handle;
    /* +0x04 */ Vec3   m_vec4;           // ctor: 0
    /* +0x10 */ int    m_int16;          // not written by the constructor
    // [UNRESOLVED] a time; BBFire weighs an unseen target by how long ago this was.
    /* +0x14 */ double m_double20;       // not written by the constructor
    /* +0x1C */ Vec3   m_vec28;          // ctor: 0
    /* +0x28 */ Vec3   m_vec40;          // ctor: 0
    /* +0x34 */ double m_double52;       // not written by the constructor; event_heardObject: the time
    /* +0x3C */ double m_double60;       // ctor: -1000.0
    /* +0x44 */ char   m_gap68[4];       // not written by the constructor
    // [UNRESOLVED] BBFire skips a target flagged 0x20 while this is negative.
    /* +0x48 */ int    m_int72;          // not written by the constructor
    /* +0x4C */ unsigned int m_flags;    // see the enum
    /* +0x50 */ int    m_int80;          // ctor: 2
    /* +0x54 */ double m_double84;       // ctor: 0.0; BBFire::chooseWeapon: a time
    /* +0x5C */ double m_double92;       // ctor: 0.0; event_heardObject: the time
    /* +0x64 */ bool   m_flag100;        // ctor: false
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SENSEDOBJECT_H
