#include "BAPALookAtObject.h"

#include "BAPCondition.h"
#include "../AIBot/BotMain.h"                    // Bot
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIControlInfo.h"
#include "../Main/AIDllFrontier.h"
#include "../../../../Math/Mat4.h"
#include <cfloat>
#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

BAPALookAtObject::BAPALookAtObject(Bot* bot, unsigned int target, float f, BAPCondition* until,
                                   unsigned long long controls, int planId,
                                   BAPVariableVec3* variable, float f2)
    : BAPALook(f, until, controls, planId)
    , m_bot(bot)
    , m_target(target)
    , m_variable(variable)
    , m_float60(f2)
{
    m_until->m_owner = this;
}

BAPALookAtObject::~BAPALookAtObject()
{
    if (m_until)
        delete m_until;
}

// From the equipment's camera to the target (raised by m_float60), normalised. Without the
// target: (1, 0, 0) and not valid.
Vec3 BAPALookAtObject::getLookDirection(bool& valid)
{
    // no null guard on the equipment, as shipped
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    Information* const target = informationManager->getInformation(m_target);
    valid = target != 0;
    Vec3 direction;
    if (!valid)
    {
        direction.x = 1.0f;
        direction.y = 0.0f;
        direction.z = 0.0f;
        return direction;
    }

    const float* const camera = &equipment->getControlInfo()->getCameraTransformation().m[12];
    Vec3 position;
    if (m_variable)
    {
        // [FRONTIER] [TEMP] [RETIRE-WHEN: aidll-02-skeleton] BAPVariable<Vec3> (value at +8) is not
        // recovered; no recovered caller passes one.
        BF2_AI_FRONTIER("dice::hfe::ai::BAPALookAtObject::getLookDirection (variable)");
    }
    else
    {
        position = target->getPosition();
    }
    position.y += m_float60;

    direction.x = position.x - camera[0];
    direction.y = position.y - camera[1];
    direction.z = position.z - camera[2];
    const float lengthSquared =
        direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;
    if (std::fabs(lengthSquared - 1.0f) < FLT_EPSILON)
        return direction;
    if (std::fabs(lengthSquared) < FLT_EPSILON)
    {
        direction.x = 0.0f;
        direction.y = 0.0f;
        direction.z = 0.0f;
        return direction;
    }
    const float scale = 1.0f / std::sqrt(lengthSquared);
    direction.x *= scale;
    direction.y *= scale;
    direction.z *= scale;
    return direction;
}

} // namespace ai
} // namespace hfe
} // namespace dice
