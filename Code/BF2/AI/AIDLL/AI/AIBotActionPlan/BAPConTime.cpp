#include "BAPConTime.h"

#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1facb0 (Ghidra).
BAPConTime::BAPConTime(double delay, bool b, bool started, bool expired)
    : m_delay(delay)
    , m_started(started || expired)
    , m_bool19(b)
{
    m_end = expired ? -1.0
                    : static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime() + delay;
}

BAPConTime::~BAPConTime()
{
}

// 0x1fae70 (Ghidra).
bool BAPConTime::evaluate(Bot*)
{
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    if (!m_started)
    {
        m_started = true;
        m_end = now + m_delay;
    }
    if (m_end <= now)
        m_started = false;
    return m_end <= now;
}

} // namespace ai
} // namespace hfe
} // namespace dice
