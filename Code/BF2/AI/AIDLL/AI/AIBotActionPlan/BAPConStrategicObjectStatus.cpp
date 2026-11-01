#include "BAPConStrategicObjectStatus.h"

#include "../AIStrategicEntity/AIStrategicObject.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1faac0 (Ghidra).
BAPConStrategicObjectStatus::BAPConStrategicObjectStatus(AIStrategicObject* object, int side, bool negate)
    : m_object(object)
    , m_side(side)
    , m_negate(negate)
{
}

BAPConStrategicObjectStatus::~BAPConStrategicObjectStatus()
{
}

// 0x1fab40 (Ghidra).
bool BAPConStrategicObjectStatus::evaluate(Bot*)
{
    const bool hasStatus = m_object->getStatus(m_side) != 0;
    return m_negate ? !hasStatus : hasStatus;
}

} // namespace ai
} // namespace hfe
} // namespace dice
