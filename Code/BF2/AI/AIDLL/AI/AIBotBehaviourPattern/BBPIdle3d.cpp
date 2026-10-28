#include "BBPIdle3d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPIdle3d::BBPIdle3d(int behaviourId)
    : BBPIdle(behaviourId)
{
    m_int10 = new int[getNumberOfBots()];
    const int numberOfBots = getNumberOfBots();
    m_float0C = new float[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_float0C[i] = 100.0f;
        m_int10[i] = -1;
    }
}

BBPIdle3d::~BBPIdle3d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdle3d::~BBPIdle3d");
}

BotScript* BBPIdle3d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdle3d::createPlan");
}

int BBPIdle3d::getType()
{
    return 2;
}

} // namespace ai
} // namespace hfe
} // namespace dice
