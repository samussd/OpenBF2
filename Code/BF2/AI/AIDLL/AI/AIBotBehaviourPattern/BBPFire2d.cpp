#include "BBPFire2d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPFire2d::BBPFire2d(int behaviourId)
    : BBPFire()
{
    m_behaviourId = behaviourId;
    m_bool28 = new bool[getNumberOfBots()];
}

BBPFire2d::~BBPFire2d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire2d::~BBPFire2d");
}

BotScript* BBPFire2d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire2d::createPlan");
}

void BBPFire2d::init(Bot*, unsigned int, int, bool, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire2d::init");
}

void BBPFire2d::init(Bot*, unsigned int, const Vec3&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire2d::init");
}

int BBPFire2d::getImplementationType()
{
    return 2;
}

} // namespace ai
} // namespace hfe
} // namespace dice
