// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining CommunicationHub operations.

#include "CommunicationHub.h"

#include <cmath>

#include "AIDllFrontier.h"
#include "IBotManager.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/CommunicationAgent.h"
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../../AIInterface/BFKnowledge/BFRadioComm.h"
#include "../../../AIInterface/Module/BFConsole.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/BFGame.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "CommonSingletonServer.h"
#include "../AIStrategicEntity/AIStrategicArea.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/Information.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"   // reset sizes by the area count

namespace dice {
namespace hfe {
namespace ai {

CommunicationHub* CommunicationHub::m_instance = 0;

// Constructs two empty strength vectors and publishes only the first hub instance.
// Call reset before using the three timestamps, which construction leaves uninitialized.
CommunicationHub::CommunicationHub()
{
    // m_time4, m_time12 and m_time44 deliberately not initialised - the binary writes none of
    // them. The two vectors default-construct on their own.
    if (!m_instance)
        m_instance = this;
}

// it does test `== this` before nulling the static, unlike `AerialHeightMap` one step earlier.
// The two vectors are then torn down in reverse, which is the compiler's own work.
CommunicationHub::~CommunicationHub()
{
    if (m_instance == this)
        m_instance = 0;
}

// [FRONTIER] Behavior methods stop until their strategic-object dependencies are implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// Stamps the three times with -1000 and gives every strategic area a (6, -1000) strength entry
// on both sides. The first reset appends the entries; later ones overwrite them in place,
// unchecked, so the area count must not have grown.
void CommunicationHub::reset()
{
    const double never = -1000.0;
    m_time4 = never;
    m_time12 = never;
    m_time44 = never;

    const int areas = static_cast<int>(
        AIStrategicObjectManager::getInstance()->getStrategicAreas().size());
    const CombatStrengthType unknown = static_cast<CombatStrengthType>(6);
    if (m_combatStrength[0].empty())
    {
        for (int i = 0; i < areas; ++i)
        {
            m_combatStrength[0].push_back(std::make_pair(unknown, never));
            m_combatStrength[1].push_back(std::make_pair(unknown, never));
        }
    }
    else
    {
        for (int i = 0; i < areas; ++i)
        {
            m_combatStrength[0][i].first = unknown;
            m_combatStrength[1][i].first = unknown;
            m_combatStrength[0][i].second = never;
            m_combatStrength[1][i].second = never;
        }
    }
}

// 0x3f57e0 (Ghidra). A bot saw an enemy primary object near a strategic area. Aircraft (strength
// types 4 and 5) count within 100 m of an area and are reported at most once a minute in all;
// anything else within 30 m, at most once a minute per area and side, sooner when a soldier
// (type 2) follows a type 0 or 1. The engine takes the report.
void CommunicationHub::enemySpottedByAI(IAIPlayer* player, IAIObject* object, int side)
{
    Information* const information = informationManager->getInformation(object->getInformation(side));
    // no null guard, as shipped
    if (!information->isPrimary() || !information->getPhysical())
        return;
    const int type = information->getPhysical()->getStrengthType();
    const std::map<std::string, AIStrategicArea*>& areas = AIStrategicObjectManager::getInstance()->getStrategicAreas();
    const bool aircraft = static_cast<unsigned int>(type - 4) < 2;
    const float range = aircraft ? 10000.0f : 900.0f;
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    int index = 0;
    for (std::map<std::string, AIStrategicArea*>::const_iterator it = areas.begin(); it != areas.end(); ++it, ++index)
    {
        Vec2 at;
        at.x = 0.0f;
        at.y = 0.0f;
        vec3ToVec2XZ(object->getPosition(), at);
        const Vec2& centre = it->second->getCenterPosition();
        const float dx = at.x - centre.x;
        const float dz = at.y - centre.y;
        if (!(dz * dz + dx * dx < range))
            continue;
        bool report = false;
        if (aircraft && 60.0 < now - m_time44)
        {
            m_time44 = now;
            report = true;
        }
        else
        {
            std::pair<CombatStrengthType, double>& record = m_combatStrength[side - 1][index];
            if (60.0 < now - record.second)
            {
                record.first = static_cast<CombatStrengthType>(type);
                record.second = now;
                report = true;
            }
            else if ((record.first == 0 || record.first == 1) && type == 2)
            {
                record.first = static_cast<CombatStrengthType>(2);
                record.second = now;
                report = true;
            }
        }
        if (report)
        {
            static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)
                ->enemySpottedByAI(player->getPlayer(), object->getObject());
            return;
        }
    }
}

// Refreshes what the side knows about an object a human spotted.
void CommunicationHub::enemySpottedByHuman(IAIObject* object, unsigned char side) const
{
    Information* information = informationManager->getInformation(object->getInformation(side));
    if (!information)
        information = informationManager->getInformation(object->forceGetInformation(side, true));
    information->setTime(static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime());
}

// Type 2 goes on the air once in twenty seconds per side; in between it only reaches the bot it
// names, and only that bot's own ears.
void CommunicationHub::filterAndSendAIRadioMessage(BotMain*, int playerId,
                                                   ChannelMessageType messageChannel,
                                                   AIRadioChannelType channel,
                                                   AIRadioMessageType type,
                                                   unsigned char voice, int senderId,
                                                   unsigned char side, unsigned char value)
{
    const IBFTimer* const timer = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    if (static_cast<int>(type) == 2)
    {
        double* const lastSent = &m_time4 + (side - 1);
        if (timer->getGameTime() - *lastSent < 20.0)
        {
            IAIPlayer* const player =
                static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
                    ->getPlayerFromId(value);
            if (!player || !player->getIsAIPlayer())
                return;
            IBFModuleCourier* const courier =
                static_cast<IBFModuleCourier*>(CommonSingletonServer::m_iBFModuleCourier);
            int sameVoice;
            AIRadioMessageSettings settings;
            courier->getVoiceFromAiMessageType(static_cast<AIRadioMessageType>(2), sameVoice);
            courier->getAIMessageSettings(sameVoice, settings);
            filterAndReceiveAIRadioMessage(settings, senderId, value, true);
            return;
        }
        *lastSent = timer->getGameTime();
    }
    static_cast<IBFRadioComm*>(CommonSingletonServer::m_iBFRadioComm)
        ->sendAIRadioMessage(playerId, messageChannel, channel, voice, senderId, side, value);
}

// A bot does not hear itself. A human's types 0 and 1 close or open the vehicle under them to
// bots, and type 2 reaches the bot only if the human is looking its way.
void CommunicationHub::filterAndReceiveAIRadioMessage(const AIRadioMessageSettings& settings,
                                                      int senderId, int receiverId, bool self)
{
    Bot* const bot = IBotManager::instance->getBotFromPlayerId(receiverId);
    if (senderId == bot->getPlayerId())
        return;

    if (settings.m_type >= 0 && settings.m_type <= 2)
    {
        IAIPlayer* const sender = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
                                      ->getPlayerFromId(senderId);
        if (sender && !sender->getIsAIPlayer())
        {
            if (settings.m_type == 2)
            {
                const float* const camera = static_cast<const float*>(sender->getCameraTransformation());
                const Vec3& position = informationManager->getInformation(bot->getCurrentEquipment())
                                           ->getPosition();
                float x = position.x - camera[12];
                float y = position.y - camera[13];
                float z = position.z - camera[14];
                const float lengthSq = x * x + y * y + z * z;
                if (std::fabs(lengthSq - 1.0f) >= 1.1920929e-07f)
                {
                    if (std::fabs(lengthSq) >= 1.1920929e-07f)
                    {
                        const float scale = 1.0f / std::sqrt(lengthSq);
                        x *= scale;
                        y *= scale;
                        z *= scale;
                    }
                    else
                    {
                        x = 0.0f;
                        y = 0.0f;
                        z = 0.0f;
                    }
                }
                float dot = camera[8] * x + camera[9] * y + camera[10] * z;
                if (dot < -1.0f)
                    dot = -1.0f;
                else if (dot > 1.0f)
                    dot = 1.0f;
                if (std::acos(dot) > 1.5707964f)
                    return;
            }
            else
            {
                IAIObject* const primary = sender->getCurrentVehicle()->getPrimaryObject();
                if (primary)
                    primary->setIsAllowedByBots(settings.m_type == 1);
            }
        }
    }

    bot->getCommunicationAgent()->receiveRadioMessage(settings, senderId, self);
}

} // namespace ai
} // namespace hfe
} // namespace dice
