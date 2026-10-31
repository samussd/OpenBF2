#include "BAPAMoveToObject.h"

#include "BAPCondition.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/BotNavigation.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../Main/AIDllFrontier.h"

#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

// 0x1ea5d0 (Ghidra).
BAPAMoveToObject::BAPAMoveToObject(Bot* bot, unsigned int object, float radius, float maxSpeed,
                                   float maxArriveSpeed, BAPCondition* finishCondition,
                                   unsigned long long controls, int planId)
    : BAPAMoveTo(bot)
    , m_object(object)
    , m_radius(radius)
    , m_radiusSqr(radius * radius)
{
    m_maxSpeed = maxSpeed;
    m_maxArriveSpeed = maxArriveSpeed;
    m_controls = controls;
    m_planId = planId;
    m_finishCondition = finishCondition;
    if (finishCondition)
        finishCondition->m_owner = this;
}

BAPAMoveToObject::~BAPAMoveToObject()
{
}

// 0x1ea7d0 (Ghidra). Runs only through an interpreter entry.
bool BAPAMoveToObject::execute(Bot*, bool& ok, bool& done)
{
    ok = false;
    done = true;
    return false;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPAMoveToObject::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPAMoveToObject::streamSelf");
}

unsigned long long BAPAMoveToObject::getControls()
{
    return m_controls;
}

BAPStatementType BAPAMoveToObject::getId() const
{
    return BAPSTMoveToObject;
}

bool BAPAMoveToObject::hasDefault()
{
    return false;
}

// 0x1ea750 (Ghidra). Finishing clears the navigation's progress.
bool BAPAMoveToObject::isFinished(Bot* bot)
{
    if ((!m_finishCondition || !m_finishCondition->evaluate(bot)) && m_radiusSqr <= getSimpleTargetDistanceSqr(bot))
        return false;
    static_cast<BotMain*>(bot)->getBotNavigation()->clearProgress();
    return true;
}

unsigned int BAPAMoveToObject::getObject()
{
    return m_object;
}

// 0x1ea7f0 (Ghidra). No null guards, as shipped.
float BAPAMoveToObject::getSimpleTargetDistance(Bot* bot)
{
    return std::sqrt(getSimpleTargetDistanceSqr(bot));
}

// 0x1ea8e0 (Ghidra). No null guards, as shipped.
float BAPAMoveToObject::getSimpleTargetDistanceSqr(Bot* bot)
{
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    Information* const object = informationManager->getInformation(m_object);
    const Vec3& from = equipment->getPosition();
    const Vec3& to = object->getPosition();
    const float x = from.x - to.x;
    const float y = from.y - to.y;
    const float z = from.z - to.z;
    return z * z + x * x + y * y;
}

} // namespace ai
} // namespace hfe
} // namespace dice
