#include "BBRandom.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/BotNavigation.h"
#include "../AIBotBehaviourPattern/BBPRandom.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFObjectEnvironment.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include <vector>
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBRandom::BBRandom(unsigned char behaviourId, unsigned char mode, PlanGenerator* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    m_grenadePosition.x = 0.0f;
    m_grenadePosition.y = 0.0f;
    m_grenadePosition.z = 0.0f;
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
    }
    m_grenadePending = false;
    m_grenadeNoticed = 0.0;
    m_grenadeTimeLeft = 0.0;
}

BBRandom::~BBRandom()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBRandom::~BBRandom");
}

// 0x22a780 (Ghidra). Urgent (1000) once a flagged grenade warning's fuse has run out, for five
// seconds; otherwise 0 and its plan let go. The first bot to see a flagged warning records it
// for all. A refresh without the full evaluation keeps the urgency while the plan runs. The
// shipped body also traces a line from a metre above the bot to the grenade and drops the result.
float BBRandom::calculateUrgency(Bot* bot, bool full, float)
{
    if (!full)
    {
        if (m_generator->isInitiated(bot))
            return m_urgency[bot->getBotId()];
        m_generator->unInit(bot);
        m_urgency[bot->getBotId()] = 0.0f;
        return 0.0f;
    }

    const float quiet = 0.0f;
    Vec3 grenade;
    grenade.x = 0.0f;
    grenade.y = 0.0f;
    grenade.z = 0.0f;
    double timeLeft;
    if (!m_grenadePending)
    {
        if (bot->getBotNavigation()->getGrenadeWarning(grenade, timeLeft)
            && bot->getBotNavigation()->getGrenadeFlag())
        {
            m_grenadePending = true;
            m_grenadeNoticed = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
            m_grenadeTimeLeft = timeLeft;
            m_grenadePosition = grenade;
        }
    }
    else
    {
        const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
        const double fuse = m_grenadeTimeLeft + m_grenadeNoticed;
        if (now > fuse)
        {
            if (now > fuse + 5.0f)
                m_grenadePending = false;
            std::vector<IAIObject*> ignore;
            // no null guard, as shipped
            Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
            ignore.push_back(equipment->getAIObject());
            IAIObjectEnvironment* const environment =
                static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment);
            const Vec3& position = equipment->getPosition();
            Vec3 eye;
            eye.x = position.x + 0.0f;
            eye.y = position.y + 1.0f;
            eye.z = position.z + 0.0f;
            bool hit;
            environment->collideLineWithWorld(eye, m_grenadePosition, ignore, false, 0.7f, false, hit);
            static_cast<BBPRandom*>(m_generator)->init(bot);
            m_urgency[bot->getBotId()] = 1000.0f;
            return 1000.0f;
        }
    }

    if (quiet < 0.001f)
        m_generator->unInit(bot);
    else
        static_cast<BBPRandom*>(m_generator)->init(bot);
    m_urgency[bot->getBotId()] = quiet;
    return m_urgency[bot->getBotId()];
}

// 0x22aaf0 (Ghidra). The same shipped body as BBTriggerable::isApplicable: never applicable.
bool BBRandom::isApplicable(Information*, Bot* bot)
{
    if (bot->getPlayer()->getIsAlive())
        bot->getManDown();
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
