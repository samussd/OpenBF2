#include "BBPFire3d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPFire3d::BBPFire3d(int behaviourId)
    : BBPFire()
{
    m_behaviourId = behaviourId;
}

BBPFire3d::~BBPFire3d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire3d::~BBPFire3d");
}

BotScript* BBPFire3d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire3d::createPlan");
}

void BBPFire3d::init(Bot*, unsigned int, int, bool, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire3d::init");
}

void BBPFire3d::init(Bot*, unsigned int, const Vec3&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFire3d::init");
}

int BBPFire3d::getImplementationType()
{
    return 4;
}

} // namespace ai
} // namespace hfe
} // namespace dice
