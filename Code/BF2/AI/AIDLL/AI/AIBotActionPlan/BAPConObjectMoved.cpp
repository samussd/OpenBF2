#include "BAPConObjectMoved.h"

#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1ffb50 (Ghidra).
BAPConObjectMoved::BAPConObjectMoved(unsigned int object, float distance)
    : BAPConObject(object)
{
    m_lastPosition.x = 0.0f;
    m_lastPosition.y = 0.0f;
    m_lastPosition.z = 0.0f;
    m_distanceSqr = distance * distance;
}

BAPConObjectMoved::~BAPConObjectMoved()
{
}

// 0x1ffbf0 (Ghidra). A gone object has not moved.
bool BAPConObjectMoved::evaluate(Bot*)
{
    Information* const object = informationManager->getInformation(m_object);
    if (!object)
        return false;
    const Vec3& at = object->getPosition();
    const float dx = at.x - m_lastPosition.x;
    const float dy = at.y - m_lastPosition.y;
    const float dz = at.z - m_lastPosition.z;
    if (!(m_distanceSqr < dx * dx + dy * dy + dz * dz))
        return false;
    m_lastPosition = object->getPosition();
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
