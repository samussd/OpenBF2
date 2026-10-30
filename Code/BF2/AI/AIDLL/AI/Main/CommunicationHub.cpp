// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining CommunicationHub operations.

#include "CommunicationHub.h"

#include "AIDllFrontier.h"
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

void CommunicationHub::enemySpottedByHuman(IAIObject*, unsigned char) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationHub::enemySpottedByHuman");
}

void CommunicationHub::filterAndSendAIRadioMessage(BotMain*, int, ChannelMessageType,
                                                   AIRadioChannelType, AIRadioMessageType,
                                                   unsigned char, int, unsigned char,
                                                   unsigned char)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationHub::filterAndSendAIRadioMessage");
}

void CommunicationHub::filterAndReceiveAIRadioMessage(const AIRadioMessageSettings&, int, int, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationHub::filterAndReceiveAIRadioMessage");
}

} // namespace ai
} // namespace hfe
} // namespace dice
