#include "BBPFire.h"

#include "../Main/AIDllFrontier.h"

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
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire::~BBPFire");
}

int BBPFire::getImplementationType()
{
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
