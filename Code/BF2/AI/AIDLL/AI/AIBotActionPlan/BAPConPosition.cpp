#include "BAPConPosition.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1f98e0 (Ghidra).
BAPConPosition::BAPConPosition(const Vec3& position, float radius)
{
    m_position = new float[3];
    m_position[0] = position.x;
    m_position[1] = position.y;
    m_position[2] = position.z;
    m_hasHeight = true;
    m_radiusSqr = radius * radius;
}

// 0x1f99c0 (Ghidra).
BAPConPosition::BAPConPosition(const Vec2& position, float radius)
{
    m_position = new float[2];
    m_position[0] = position.x;
    m_position[1] = position.y;
    m_hasHeight = false;
    m_radiusSqr = radius * radius;
}

BAPConPosition::~BAPConPosition()
{
    delete[] m_position;
}

// 0x1f9b90 (Ghidra).
bool BAPConPosition::evaluate(Bot* bot)
{
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    if (!equipment)
        return true;
    const Vec3& at = equipment->getPosition();
    if (!m_hasHeight)
    {
        const float dx = at.x - m_position[0];
        const float dz = at.z - m_position[1];
        return dz * dz + dx * dx < m_radiusSqr;
    }
    const float dx = at.x - m_position[0];
    const float dy = at.y - m_position[1];
    const float dz = at.z - m_position[2];
    return dx * dx + dy * dy + dz * dz <= m_radiusSqr;
}

} // namespace ai
} // namespace hfe
} // namespace dice
