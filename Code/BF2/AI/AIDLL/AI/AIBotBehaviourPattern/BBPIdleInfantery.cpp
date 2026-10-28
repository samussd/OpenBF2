#include "BBPIdleInfantery.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPIdleInfantery::BBPIdleInfantery(int behaviourId)
    : BBPIdle(behaviourId)
{
}

BBPIdleInfantery::~BBPIdleInfantery()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdleInfantery::~BBPIdleInfantery");
}

BotScript* BBPIdleInfantery::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPIdleInfantery::createPlan");
}

int BBPIdleInfantery::getType()
{
    return 3;
}

} // namespace ai
} // namespace hfe
} // namespace dice
