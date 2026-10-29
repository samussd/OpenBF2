#include "BAPALookInDir.h"

#include "BAPCondition.h"

namespace dice {
namespace hfe {
namespace ai {

BAPALookInDir::BAPALookInDir(const Vec3& direction, float f, BAPCondition* until,
                             unsigned long long controls, int planId)
    : BAPALook(f, until, controls, planId)
    , m_direction(direction)
{
    m_until->m_owner = this;
}

BAPALookInDir::~BAPALookInDir()
{
    if (m_until)
        delete m_until;
}

Vec3 BAPALookInDir::getLookDirection(bool& valid)
{
    valid = true;
    return m_direction;
}

} // namespace ai
} // namespace hfe
} // namespace dice
