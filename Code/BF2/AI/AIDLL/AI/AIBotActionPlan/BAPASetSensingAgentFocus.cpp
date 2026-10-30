#include "BAPASetSensingAgentFocus.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/SensingAgent.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1f0200 (Ghidra).
BAPASetSensingAgentFocus::BAPASetSensingAgentFocus(float weight, unsigned int target, int planId, bool force)
    : BAPAction(planId)
    , m_weight(weight)
    , m_force(force)
    , m_target(target)
    , m_isTarget(true)
{
    m_direction.x = 0.0f;
    m_direction.y = 0.0f;
    m_direction.z = 0.0f;
}

// 0x1f0300 (Ghidra). m_target stays unset, as shipped.
BAPASetSensingAgentFocus::BAPASetSensingAgentFocus(float weight, const Vec3& direction, int planId, bool force)
    : BAPAction(planId)
    , m_weight(weight)
    , m_force(force)
    , m_direction(direction)
    , m_isTarget(false)
{
}

BAPASetSensingAgentFocus::~BAPASetSensingAgentFocus()
{
}

// 0x1f04f0 (Ghidra).
bool BAPASetSensingAgentFocus::execute(Bot* bot, bool& ok, bool& done)
{
    if (m_isTarget)
        bot->getSensingAgent()->insertPlanTargetInformation(m_weight, m_target, m_force);
    else
        bot->getSensingAgent()->insertPlanDirectionInformation(m_weight, m_direction, m_force);
    ok = true;
    done = true;
    return true;
}

BAPStatementType BAPASetSensingAgentFocus::getId() const
{
    return BAPSTSetSensingAgentFocus;
}

bool BAPASetSensingAgentFocus::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
