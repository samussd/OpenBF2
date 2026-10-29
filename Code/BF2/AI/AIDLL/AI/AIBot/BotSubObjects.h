// Aim-correction state used by BotMain.

#ifndef AI_AIDLL_AI_AIBOT_BOTSUBOBJECTS_H
#define AI_AIDLL_AI_AIBOT_BOTSUBOBJECTS_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BotMain;

// Aim-correction state; construction leaves the first four bytes uninitialized.
class FireCorrectionData
{
public:
    FireCorrectionData();                       // for the class
    // BotMain::planExecution's inlined stores when a projectile the bot fired passes its point.
    void setProjectile(const Vec3& pre, const Vec3& post, const Vec3& target, float maxHeight);

private:
    /*  0 */ char m_gap0[4];                    // [UNRESOLVED] - not initialised by the ctor
    /*  4 */ bool m_flag4;                      // = true
    /*  5 */ bool m_flag5;                      // = true
    /*  6 */ char m_pad6[2];
    /*  8 */ int  m_zero8[15];                  // +8..+64, all zeroed
    /* 68 */ char m_gap68[16];                  // [UNRESOLVED] - the ctor skips +68..+83
    /* 84 */ bool m_flag84;                     // = false; setProjectile: true
    /* 85 */ bool m_flag85;                     // setProjectile: true
    /* 86 */ char m_pad86[2];
    /* 88 */ Vec3 m_projectilePre;              // zeroed
    /* 100 */ Vec3 m_projectilePost;            // zeroed
    /* 112 */ Vec3 m_projectileTarget;          // zeroed
    /* 124 */ float m_projectileMaxHeight;      // not initialised by the ctor
};


} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTSUBOBJECTS_H
