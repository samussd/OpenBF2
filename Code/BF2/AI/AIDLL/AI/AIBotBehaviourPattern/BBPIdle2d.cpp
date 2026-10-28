#include "BBPIdle2d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPIdle2d::BBPIdle2d(int behaviourId)
    : BBPIdle(behaviourId)
{
    m_int0C = new int[getNumberOfBots()];
    // The bound is re-read from the settings on every pass.
    for (int i = 0; i < getNumberOfBots(); ++i)
        m_int0C[i] = -1;
}

BBPIdle2d::~BBPIdle2d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdle2d::~BBPIdle2d");
}

BotScript* BBPIdle2d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdle2d::createPlan");
}

int BBPIdle2d::getType()
{
    return 1;
}

} // namespace ai
} // namespace hfe
} // namespace dice
