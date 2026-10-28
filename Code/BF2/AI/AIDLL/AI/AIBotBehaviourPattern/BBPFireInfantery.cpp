#include "BBPFireInfantery.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBPFireInfantery::FiringState::FiringState()
{
    m_int00 = -1;
    m_int04 = -1;
    m_int08 = -1;
    m_flag0C = false;
    for (int i = 0; i < 4; ++i)
        m_flags14[i] = false;
    m_vec18.x = 0.0f;
    m_vec18.y = 0.0f;
    m_vec18.z = 0.0f;
    for (int i = 0; i < 3; ++i)
        m_flags24[i] = false;
    m_vec28.x = 0.0f;
    m_vec28.y = 0.0f;
    m_vec28.z = 0.0f;
    m_float34 = 0.0f;
}

BBPFireInfantery::Data::Data()
{
    m_int08 = 0;
    m_int10 = 0;
    m_double14 = 0.0;
    m_double1C = 10.0;
    m_vec24.x = m_vec24.y = m_vec24.z = 0.0f;
    m_vec30.x = m_vec30.y = m_vec30.z = 0.0f;
    m_vec3C.x = m_vec3C.y = m_vec3C.z = 0.0f;
    m_int4C = -1;
    m_int50 = -1;
}

BBPFireInfantery::BBPFireInfantery(int behaviourId)
    : BBPFire()
{
    m_behaviourId = behaviourId;
    m_firingStates = new FiringState[getNumberOfBots()];
    m_data = new Data[getNumberOfBots()];
}

BBPFireInfantery::~BBPFireInfantery()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFireInfantery::~BBPFireInfantery");
}

BotScript* BBPFireInfantery::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFireInfantery::createPlan");
}

void BBPFireInfantery::init(Bot*, unsigned int, int, bool, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFireInfantery::init");
}

void BBPFireInfantery::init(Bot*, unsigned int, const Vec3&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFireInfantery::init");
}

int BBPFireInfantery::getImplementationType()
{
    return 5;
}

} // namespace ai
} // namespace hfe
} // namespace dice
