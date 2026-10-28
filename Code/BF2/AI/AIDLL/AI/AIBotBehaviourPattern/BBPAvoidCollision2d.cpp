#include "BBPAvoidCollision2d.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPAvoidCollision2d::BBPAvoidCollision2d(int behaviourId)
    : BBPAvoidCollision()
{
    m_behaviourId = behaviourId;
    const int numberOfBots = getNumberOfBots();
    m_vec1C = new Math::Vec2[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_vec1C[i].x = 0.0f;
        m_vec1C[i].y = 0.0f;
    }
}

BBPAvoidCollision2d::~BBPAvoidCollision2d()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollision2d::~BBPAvoidCollision2d");
}

BotScript* BBPAvoidCollision2d::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollision2d::createPlan");
}

void BBPAvoidCollision2d::init(Bot*, Vec3&, unsigned int, Vec3&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollision2d::init");
}

} // namespace ai
} // namespace hfe
} // namespace dice
