#include "BBMedicAssist.h"

#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"
#include "../AIBot/BotMain.h"                  // Bot::getManDown
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

BBMedicAssist::BBMedicAssist(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    for (int i = 0; i < 3; ++i)
        m_words24[i] = 0;
    const int strengths = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getNBattleStrengths();
    m_int30 = new int[strengths];
    m_int34 = new int[strengths];
    m_int38 = new int[strengths];
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
    }
}

BBMedicAssist::~BBMedicAssist()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMedicAssist::~BBMedicAssist");
}

float BBMedicAssist::calculateUrgency(Bot*, bool, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMedicAssist::calculateUrgency");
}

bool BBMedicAssist::isAplicable(Information* information, Bot* bot)
{
    IPIArmament* const armament = information->getArmament();
    if (bot->getManDown() || !armament)
        return false;
    const std::vector<unsigned int>& weapons = armament->getWeapons();
    for (unsigned int i = 0; i < weapons.size(); ++i)
    {
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
        if (weapon && static_cast<unsigned int>(weapon->getWeaponType() - 1) < 2)
            return true;
    }
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
