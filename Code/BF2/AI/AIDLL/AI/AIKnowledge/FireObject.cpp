#include "FireObject.h"

#include "../AIUtils/checkPointer.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice {
namespace hfe {
namespace ai {

FireObject::FireObject(Vec3 from, Vec3 to, float* strengths, bool flag, double time, float fadeRate)
    : m_flag(flag)
    , m_from(from)
    , m_to(to)
    , m_time(time)
    , m_fadeRate(fadeRate)
{
    IAISettings* const settings =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    const int count = settings->getNBattleStrengths();
    m_strengths = checkPointer(new float[count]);
    for (int i = 0; i < count; ++i)
        m_strengths[i] = strengths[i];
}

FireObject::~FireObject()
{
    delete[] m_strengths;
}

float FireObject::getBattleStrength(int index)
{
    const float age = static_cast<float>(
        static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime() - m_time);
    return m_strengths[index] / (age * m_fadeRate + 1.0f);
}

} // namespace ai
} // namespace hfe
} // namespace dice
