#include "BBPFire.h"

namespace dice {
namespace hfe {
namespace ai {

void BBPFire::setUrgencyGenerator(BotBehaviour* behaviour)
{
    m_urgencyGenerator = behaviour;
}

BBPFire::BBPFire()
    : BBPPattern()
{
    m_targets = new int[getNumberOfBots()];
    m_changedTarget = new bool[getNumberOfBots()];
    m_weapons = new int[getNumberOfBots()];
    m_positionTargets = new int[getNumberOfBots()];
    const int numberOfBots = getNumberOfBots();
    m_positions = new Vec3[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_positions[i].x = 0.0f;
        m_positions[i].y = 0.0f;
        m_positions[i].z = 0.0f;
    }
}

BBPFire::~BBPFire()
{
    delete[] m_targets;
    delete[] m_changedTarget;
    delete[] m_weapons;
    delete[] m_positionTargets;
    delete[] m_positions;
}

int BBPFire::getImplementationType()
{
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
