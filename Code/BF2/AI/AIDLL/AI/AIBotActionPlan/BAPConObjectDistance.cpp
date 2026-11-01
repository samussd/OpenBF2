#include "BAPConObjectDistance.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIMobile.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1fd070 (Ghidra).
BAPConObjectDistance::BAPConObjectDistance(unsigned int object, float distance, bool estimated, bool flat,
                                           bool outside)
    : BAPConObject(object)
    , m_minSqr(0.0f)
    , m_maxSqr(distance * distance)
    , m_estimated(estimated)
    , m_flat(flat)
    , m_outside(outside)
    , m_lastSqr(0.0f)
{
}

BAPConObjectDistance::~BAPConObjectDistance()
{
}

// 0x1fd270 (Ghidra). A gone object counts as met.
bool BAPConObjectDistance::evaluate(Bot* bot)
{
    Information* const object = informationManager->getInformation(m_object);
    if (!object)
        return true;
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());

    Vec3 at;
    at.x = 0.0f;
    at.y = 0.0f;
    at.z = 0.0f;
    if (!m_estimated)
        at = object->getPosition();
    else if (IPIMobile* const mobile = object->getMobile())
        mobile->getPositionEstimate(at);
    else
        at = object->getPosition();

    const Vec3& from = equipment->getPosition();
    const float dx = at.x - from.x;
    const float dy = at.y - from.y;
    const float dz = at.z - from.z;
    m_lastSqr = m_flat ? dx * dx + dz * dz : dx * dx + dy * dy + dz * dz;
    if (m_outside)
        return m_maxSqr < m_lastSqr;
    return m_minSqr <= m_lastSqr && m_lastSqr <= m_maxSqr;
}

} // namespace ai
} // namespace hfe
} // namespace dice
