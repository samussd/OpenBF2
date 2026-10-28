#include "BBPAvoidCollisionInfantery.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPAvoidCollisionInfantery::BBPAvoidCollisionInfantery(int behaviourId)
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

BBPAvoidCollisionInfantery::~BBPAvoidCollisionInfantery()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollisionInfantery::~BBPAvoidCollisionInfantery");
}

BotScript* BBPAvoidCollisionInfantery::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollisionInfantery::createPlan");
}

void BBPAvoidCollisionInfantery::init(Bot*, Vec3&, unsigned int, Vec3&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPAvoidCollisionInfantery::init");
}

} // namespace ai
} // namespace hfe
} // namespace dice
