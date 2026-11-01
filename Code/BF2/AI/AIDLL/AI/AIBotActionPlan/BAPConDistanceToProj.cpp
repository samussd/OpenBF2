#include "BAPConDistanceToProj.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1f6eb0 (Ghidra).
BAPConDistanceToProj::BAPConDistanceToProj(IPIArmament* armament, float distance)
    : m_armament(armament)
    , m_distanceSqr(distance * distance)
{
}

BAPConDistanceToProj::~BAPConDistanceToProj()
{
}

// 0x1f6f30 (Ghidra).
bool BAPConDistanceToProj::evaluate(Bot* bot)
{
    Vec3 projectile;
    projectile.x = 0.0f;
    projectile.y = 0.0f;
    projectile.z = 0.0f;
    if (!m_armament->getProjectilePosition(projectile))
        return false;
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const Vec3& at = equipment->getPosition();
    const float dx = projectile.x - at.x;
    const float dy = projectile.y - at.y;
    const float dz = projectile.z - at.z;
    return dz * dz + dx * dx + dy * dy < m_distanceSqr;
}

} // namespace ai
} // namespace hfe
} // namespace dice
