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

void BBFireInfantery::initializeGenerator(BotMain*, int, bool, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBFireInfantery::initializeGenerator");
}

} // namespace ai
} // namespace hfe
} // namespace dice
