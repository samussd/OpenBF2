#include "BBPHeliFire3d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPHeliFire3d::BBPHeliFire3d(int behaviourId)
    : BBPFire()
{
    m_behaviourId = behaviourId;
}

BBPHeliFire3d::~BBPHeliFire3d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPHeliFire3d::~BBPHeliFire3d");
}

BotScript* BBPHeliFire3d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPHeliFire3d::createPlan");
}

void BBPHeliFire3d::init(Bot*, unsigned int, int, bool, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPHeliFire3d::init");
}

void BBPHeliFire3d::init(Bot*, unsigned int, const Vec3&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPHeliFire3d::init");
}

int BBPHeliFire3d::getImplementationType()
{
    return 4;
}

} // namespace ai
} // namespace hfe
} // namespace dice
