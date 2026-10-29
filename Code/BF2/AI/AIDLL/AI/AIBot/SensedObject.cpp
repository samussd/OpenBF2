#include "SensedObject.h"

namespace dice {
namespace hfe {
namespace ai {

SensedObject::SensedObject(unsigned int handle, bool seen, bool sided)
    : m_handle(handle)
    , m_double60(-1000.0)
    , m_int80(2)
    , m_double84(0.0)
    , m_double92(0.0)
    , m_flag100(false)
{
    m_vec4.x = 0.0f;  m_vec4.y = 0.0f;  m_vec4.z = 0.0f;
    m_vec28.x = 0.0f; m_vec28.y = 0.0f; m_vec28.z = 0.0f;
    m_vec40.x = 0.0f; m_vec40.y = 0.0f; m_vec40.z = 0.0f;

    unsigned int flags = seen ? kSeen : kFlag1;
    if (sided)
        flags |= kSided;
    if (!seen)
        flags |= kFlag8 | kFlag10;
    m_flags = flags;
}

SensedObject::~SensedObject()
{
}

void SensedObject::initStrengths()
{
}

} // namespace ai
} // namespace hfe
} // namespace dice
