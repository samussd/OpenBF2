#include "BBPAvoidCollisionInfantery.h"

#include "../AIBot/BotMain.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPAvoidCollisionInfantery::BBPAvoidCollisionInfantery(int behaviourId)
    : BBPAvoidCollision()
{
    m_behaviourId = behaviourId;
    const int numberOfBots = getNumberOfBots();
    m_contact = new Math::Vec2[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_contact[i].x = 0.0f;
        m_contact[i].y = 0.0f;
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

// 0x23c870 (Ghidra).
void BBPAvoidCollisionInfantery::init(Bot* bot, Vec3& contact, unsigned int obstacle,
                                      Vec3& relativeVelocity, float time)
{
    m_initiated[bot->getBotId()] = true;
    vec3ToVec2XZ(contact, m_contact[bot->getBotId()]);
    m_obstacle[bot->getBotId()] = obstacle;
    m_relativeVelocity[bot->getBotId()] = relativeVelocity;
    m_time[bot->getBotId()] = time;
}

} // namespace ai
} // namespace hfe
} // namespace dice
