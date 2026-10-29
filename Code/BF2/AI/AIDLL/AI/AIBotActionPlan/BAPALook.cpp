#include "BAPALook.h"

namespace dice {
namespace hfe {
namespace ai {

BAPALook::BAPALook(float f, BAPCondition* until, unsigned long long controls, int planId)
    : BAPAction(planId)
    , m_until(until)
    , m_float24(f)
    , m_float28(0.0f)
    , m_float32(0.0f)
{
    m_vec36.x = 0.0f;
    m_vec36.y = 0.0f;
    m_vec36.z = 0.0f;
    m_controls = controls;
}

BAPALook::~BAPALook()
{
}

bool BAPALook::execute(Bot*, bool& ok, bool& done)
{
    ok = false;
    done = true;
    return false;
}

BAPStatementType BAPALook::getId() const
{
    return BAPSTLookAt;
}

bool BAPALook::hasDefault()
{
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
