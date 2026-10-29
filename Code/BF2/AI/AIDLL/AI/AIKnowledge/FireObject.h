// A shot a bot has noticed: where it came from and went, when, and the per-side battle
// strengths it carried, which fade with time. BotSenses keeps them with the firer's handle.
// [INFERRED] Source placement: FireObject.cpp follows AIInformationGrid.cpp in the binary.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_FIREOBJECT_H
#define AI_AIDLL_AI_AIKNOWLEDGE_FIREOBJECT_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class FireObject
{
public:
    // [INFERRED] parameter names. The strengths are copied, one per battle strength
    // (IAISettings::getNBattleStrengths).
    FireObject(Vec3 from, Vec3 to, float* strengths, bool flag, double time, float fadeRate);
    ~FireObject();

    // The strength for one side, divided down by the time since the shot.
    float getBattleStrength(int index);
    // isWithinRadius(const Vec3&, float) (478 bytes) is not recovered: nothing calls it yet.

    /* +0x00 */ bool   m_flag;
    /* +0x04 */ float* m_strengths;
    /* +0x08 */ Vec3   m_from;
    /* +0x14 */ Vec3   m_to;          // SensingAgent::evaluateFireObjects focuses here for friendly fire
    /* +0x20 */ double m_time;
    /* +0x28 */ float  m_fadeRate;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_FIREOBJECT_H
