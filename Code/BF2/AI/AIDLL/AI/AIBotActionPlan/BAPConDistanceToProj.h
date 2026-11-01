// True while the armament's last projectile is within a distance of the bot.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONDISTANCETOPROJ_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONDISTANCETOPROJ_H

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class IPIArmament;

class BAPConDistanceToProj : public BAPCondition
{
public:
    BAPConDistanceToProj(IPIArmament* armament, float distance);
    virtual ~BAPConDistanceToProj();
    virtual bool evaluate(Bot* bot);
    virtual int getConditionType() const { return 15; }

private:
    /* +0x08 */ IPIArmament* m_armament;
    /* +0x0C */ float m_distanceSqr;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPCONDISTANCETOPROJ_H
