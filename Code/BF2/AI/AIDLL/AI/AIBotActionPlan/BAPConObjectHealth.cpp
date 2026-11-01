#include "BAPConObjectHealth.h"

#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIPhysical.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1fe400 (Ghidra).
BAPConObjectHealth::BAPConObjectHealth(unsigned int object, float health)
    : BAPConObject(object)
    , m_health(health)
{
}

BAPConObjectHealth::~BAPConObjectHealth()
{
}

// 0x1fe500 (Ghidra).
bool BAPConObjectHealth::evaluate(Bot*)
{
    Information* const object = informationManager->getInformation(m_object);
    if (!object)
        return false;
    return object->getPhysical()->getHealth() <= m_health;
}

} // namespace ai
} // namespace hfe
} // namespace dice
