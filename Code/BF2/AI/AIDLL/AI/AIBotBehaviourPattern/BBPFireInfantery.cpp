#include "BBPFireInfantery.h"

#include "../Main/AIDllFrontier.h"
#include "../AIBot/BotMain.h"   // Bot::getBotId

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
    delete[] m_firingStates;
    delete[] m_data;
}

BotScript* BBPFireInfantery::createPlan(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBPFireInfantery::createPlan");
}

// 0x26c480 (Ghidra). The two flags are not read. A plan not yet initiated starts its fire
// decisions over.
void BBPFireInfantery::init(Bot* bot, unsigned int target, int weapon, bool, bool)
{
    const unsigned int id = bot->getBotId();
    m_changedTarget[id] = static_cast<unsigned int>(m_targets[id]) != target;
    m_targets[id] = static_cast<int>(target);
    m_weapons[id] = weapon;
    if (!m_initiated[id])
    {
        m_data[id].m_int08 = 0;
        m_data[id].m_int10 = 0;
        m_data[id].m_double14 = 0.0;
        m_data[id].m_double1C = 10.0;
        m_firingStates[id].m_int00 = -1;
    }
    m_initiated[id] = true;
}

// 0x26c530 (Ghidra). Always starts the fire decisions over.
void BBPFireInfantery::init(Bot* bot, unsigned int target, const Vec3& position, int weapon)
{
    const unsigned int id = bot->getBotId();
    m_initiated[id] = true;
    m_targets[id] = -1;
    m_weapons[id] = weapon;
    m_positionTargets[id] = static_cast<int>(target);
    m_positions[id] = position;
    m_data[id].m_int08 = 0;
    m_data[id].m_int10 = 0;
    m_data[id].m_double14 = 0.0;
    m_data[id].m_double1C = 10.0;
    m_firingStates[id].m_int00 = -1;
}

int BBPFireInfantery::getImplementationType()
{
    return 5;
}

} // namespace ai
} // namespace hfe
} // namespace dice
