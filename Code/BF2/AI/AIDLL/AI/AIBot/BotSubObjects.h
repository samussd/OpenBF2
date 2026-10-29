// Aim-correction state used by BotMain.

#ifndef AI_AIDLL_AI_AIBOT_BOTSUBOBJECTS_H
#define AI_AIDLL_AI_AIBOT_BOTSUBOBJECTS_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world { class IObject; }
namespace ai {

class BotMain;

// Aim-correction state; construction leaves the first four bytes uninitialized.
class FireCorrectionData
{
public:
    FireCorrectionData();                       // for the class
    // BotMain::planExecution's inlined stores when a projectile the bot fired passes its point.
    void setProjectile(const Vec3& pre, const Vec3& post, const Vec3& target, float maxHeight);
    // BotMain::setFiringTarget: start over on a new target, keeping its engine object.
    void newTarget(unsigned int handle);
    // [INFERRED] accessor: BotMain::setFiringTarget stores +84 directly after newTarget.
    void setProjectilePassed(bool passed) { m_flag84 = passed; }

private:
    /*  0 */ char m_gap0[4];                    // [UNRESOLVED] - not initialised by the ctor
    /*  4 */ bool m_flag4;                      // = true
    /*  5 */ bool m_flag5;                      // = true
    /*  6 */ bool m_flag6;                      // not initialised by the ctor; newTarget: true
    /*  7 */ char m_pad7;
    /*  8 */ int  m_zero8[9];                   // +8..+43, all zeroed
    /* 44 */ Vec3 m_vec44;                      // zeroed; newTarget: zeroed
    /* 56 */ int  m_zero56[3];                  // zeroed
    /* 68 */ char m_gap68[4];                   // [UNRESOLVED] - the ctor skips +68..+83
    /* 72 */ double m_double72;                 // not initialised by the ctor; newTarget: -1.0
    /* 80 */ world::IObject* m_targetObject;    // not initialised by the ctor; newTarget
    /* 84 */ bool m_flag84;                     // = false; setProjectile: true; new target: false
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
