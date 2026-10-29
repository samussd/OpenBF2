#include "BBFireInfantery.h"

#include "../AIBotBehaviourPattern/BBPFireInfantery.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBFireInfantery::BBFireInfantery(unsigned char behaviourId, unsigned char mode,
                                 BBPFireInfantery* generator, UrgeCurve* urgeCurve, float* weights)
    : BBFire(behaviourId, mode, generator, urgeCurve, weights)
    , m_fireInfantery(generator)
{
}

BBFireInfantery::~BBFireInfantery()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFireInfantery::~BBFireInfantery");
}

// 0x21f2c0 (Ghidra).
void BBFireInfantery::initializeGenerator(BotMain* bot, int weapon, bool b, bool c)
{
    BBFire::initializeGenerator(bot, weapon, b, c);
}

} // namespace ai
} // namespace hfe
} // namespace dice
