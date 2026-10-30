// Points the bot's sensing agent along a direction (with a weight and whether to recompute at once)
// or at a target. Done in one step.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASETSENSINGAGENTFOCUS_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASETSENSINGAGENTFOCUS_H

#include "BAPAction.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPASetSensingAgentFocus : public BAPAction
{
public:
    BAPASetSensingAgentFocus(float weight, unsigned int target, int planId, bool force);
    BAPASetSensingAgentFocus(float weight, const Vec3& direction, int planId, bool force);
    virtual ~BAPASetSensingAgentFocus();
    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ float m_weight;
    /* +0x18 */ bool m_force;
    /* +0x1C */ unsigned int m_target;     // set by the target constructor only
    /* +0x20 */ Vec3 m_direction;
    /* +0x2C */ bool m_isTarget;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASETSENSINGAGENTFOCUS_H
