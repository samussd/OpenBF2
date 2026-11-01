#include "BAPConPointDistance.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1f8e50 (Ghidra).
BAPConPointDistance::BAPConPointDistance(const Vec3& point, float distance, bool flat)
    : m_minSqr(0.0f)
    , m_maxSqr(distance * distance)
    , m_flat(flat)
    , m_lastSqr(0.0f)
    , m_point(point)
{
}

BAPConPointDistance::~BAPConPointDistance()
{
}

// 0x1f90d0 (Ghidra).
bool BAPConPointDistance::evaluate(Bot* bot)
{
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const Vec3& at = equipment->getPosition();
    const float dx = m_point.x - at.x;
    const float dy = m_point.y - at.y;
    const float dz = m_point.z - at.z;
    m_lastSqr = m_flat ? dx * dx + dz * dz : dx * dx + dy * dy + dz * dz;
    return m_minSqr <= m_lastSqr && m_lastSqr <= m_maxSqr;
}

} // namespace ai
} // namespace hfe
} // namespace dice
