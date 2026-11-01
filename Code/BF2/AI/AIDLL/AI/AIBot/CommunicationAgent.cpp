// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining CommunicationAgent operations.

#include "CommunicationAgent.h"
#include "BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"

#include "../../../../IO/Input/PlayerInput.h"

#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/CommunicationHub.h"
#include "../../../AIInterface/Module/BFConsole.h"   // IBFModuleCourier, AIRadioMessageSettings
#include "../../../AIInterface/BFKnowledge/BFTimer.h"   // IBFTimer::getGameTime

namespace dice {
namespace hfe {
namespace ai {

// Caches voice timing settings for the eighteen radio message types.
CommunicationAgent::CommunicationAgent(BotMain* owner)
    : m_bot(owner)
{
    // eighteen. See the header block.
    m_receivedMessages.resize(18);
    m_receiveTimeToLive.resize(18);
    m_maxSendPeriod.resize(18);

    IBFModuleCourier* const courier =
        static_cast<IBFModuleCourier*>(CommonSingletonServer::m_iBFModuleCourier);

    for (unsigned int type = 0; type <= 17; ++type)
    {
        int voice;
        if (courier->getVoiceFromAiMessageType(static_cast<AIRadioMessageType>(type), voice))
        {
            AIRadioMessageSettings settings;
            courier->getAIMessageSettings(voice, settings);
            // Crossed, and in this order - see the block above.
            m_receiveTimeToLive[type] = settings.m_receiveTimeToLive;
            m_maxSendPeriod[type]     = settings.m_maxSendPeriod;
        }
        else
        {
            m_receiveTimeToLive[type] = 0.0f;
            m_maxSendPeriod[type]     = 0.0f;
        }
    }

    reset();
}

// Resets message state and acquires the shared communication hub.
// Rate-limited per type: m_lastSentTime holds the earliest time the type may be queued again.
// A type already waiting in the queue is not queued twice, but its limit still restarts.
void CommunicationAgent::enqueueRadioMessage(AIRadioMessageType type, AIRadioChannelType channel,
                                             unsigned int priority, float delay, int value)
{
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    if (!(m_lastSentTime[type] <= now))
        return;

    CAPrioMessage message;
    message.m_type = type;
    message.m_channel = channel;
    message.m_priority = priority;
    message.m_int12 = value;
    message.m_time = now + static_cast<double>(delay);
    message.m_delay = delay;
    if (m_queuedTypes.insert(std::make_pair(type, message.m_time)).second)
        m_queue.push(message);

    m_lastSentTime[type] = now + static_cast<double>(m_maxSendPeriod[type]);
}

void CommunicationAgent::reset()
{
    // The shared hub, re-read on every reset. See the block above.
    m_hub = CommunicationHub::m_instance;

    m_lastSentTime.resize(0);
    m_lastSentTime.resize(18);

    for (unsigned int type = 0; type <= 17; ++type)
        m_receivedMessages[type].clear();

    m_queuedTypes.clear();

    // No clear on a priority_queue - see the block above.
    while (!m_queue.empty())
        m_queue.pop();

    m_double60 = 0.0;
    m_int92 = -1;
    // This order is the binary's.
    m_lastHonkTime = 0.0;
    m_honkedAtTime = 0.0;
}

// 0x1d47f0 (Ghidra).
bool CommunicationAgent::getReceivedMessage(AIRadioMessageInfo& out, AIRadioMessageType type,
                                            int playerId) const
{
    const std::list<AIRadioMessageInfo>& messages = m_receivedMessages[type];
    for (std::list<AIRadioMessageInfo>::const_iterator it = messages.begin(); it != messages.end(); ++it)
    {
        if (it->m_senderPlayerId == playerId)
        {
            out = *it;
            return true;
        }
    }
    return false;
}

// 0x1d4a00 (Ghidra).
bool CommunicationAgent::receivedMessageExists(AIRadioMessageType type, int playerId)
{
    const std::list<AIRadioMessageInfo>& messages = m_receivedMessages[type];
    for (std::list<AIRadioMessageInfo>::const_iterator it = messages.begin(); it != messages.end(); ++it)
    {
        if (playerId == -1 || it->m_senderPlayerId == playerId)
            return true;
    }
    return false;
}

bool CommunicationAgent::confirmReceivedMessage(AIRadioMessageType type, int playerId)
{
    bool confirmed = false;
    std::list<AIRadioMessageInfo>& messages = m_receivedMessages[type];
    for (std::list<AIRadioMessageInfo>::iterator it = messages.begin(); it != messages.end(); ++it)
    {
        if (it->m_senderPlayerId == playerId && !it->m_bool16 && !it->m_bool17)
        {
            it->m_bool16 = true;
            confirmed = true;
        }
    }
    return confirmed;
}

// Services one bot's radio on the BotManager communication tick: old messages go every tick,
// and at most once a second the bot speaks.
void CommunicationAgent::update()
{
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    clearOutOldMessages(now);
    if (now < m_double60)
        m_double60 = now;
    if (now - m_double60 > 1.0)
    {
        m_double60 = now;
        pollForRadioMessage(now);
    }
}

// Lifts the send limits that have passed and drops what has outlived its time to live. Each
// inbox is newest first, so everything from the first expired message on goes.
void CommunicationAgent::clearOutOldMessages(double time)
{
    for (unsigned int type = 0; type <= 17; ++type)
    {
        if (m_lastSentTime[type] < time)
            m_lastSentTime[type] = 0.0;

        std::list<AIRadioMessageInfo>& messages = m_receivedMessages[type];
        std::list<AIRadioMessageInfo>::iterator it = messages.begin();
        while (it != messages.end()
               && time < it->m_double8 + static_cast<double>(m_receiveTimeToLive[type]))
            ++it;
        messages.erase(it, messages.end());
    }
}

void CommunicationAgent::pollForRadioMessage(double time)
{
    sendCommunicationAgentMessages(time);

    CAPrioMessage message;
    while (peekRadioMessage(message) && !waitWithSendingMessage(message, time))
    {
        postEvent(message);
        dequeueRadioMessage(message);
    }
}

void CommunicationAgent::sendCommunicationAgentMessages(double)
{
    Information* const information = informationManager->getInformation(m_bot->getCurrentEquipment());
    sendCallForMedic(information);
    shutOffHorn(information);
}

// A hurt soldier or a damaged vehicle, standing still, calls for help unless it carries what
// would fix it. The vehicle-free soldier asks for a medic below half health.
void CommunicationAgent::sendCallForMedic(Information* information)
{
    const int side = m_bot->getSide();
    Information* const primary = informationManager->getInformation(information->getPrimaryObject(side));
    IPIMobile* const mobile = primary->getMobile();
    const bool moving = m_bot->isAirBorn()
        || (primary && (primary->getObjectTypeFlags() & 0x10u) && mobile
            && mobile->getSpeedMagnitude() > 3.0f);
    if (!information->isPrimary() || moving)
        return;

    const float health = information->getPhysical()->getHealth();
    const int strengthType = information->getPhysical()->getStrengthType();
    const float limit = strengthType == 0 ? 0.5f : 0.75f;
    if (!(health > 0.0f) || !(health < limit))
        return;

    if (information->getArmament())
    {
        const std::vector<unsigned int>& weapons = information->getArmament()->getWeapons();
        for (unsigned int i = 0; i < weapons.size(); ++i)
        {
            WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
            if (weapon && (weapon->getWeaponType() == 1 || weapon->getWeaponType() == 3)
                && static_cast<float>(weapon->getStrength(strengthType)) > 0.0f)
                return;
        }
    }

    if (strengthType == 0)
        enqueueRadioMessage(static_cast<AIRadioMessageType>(3), static_cast<AIRadioChannelType>(0),
                            5, 0.0f, -1);
    else
        enqueueRadioMessage(static_cast<AIRadioMessageType>(4), static_cast<AIRadioChannelType>(6),
                            5, 0.0f, 0);
}

// A horn is the vehicle's fire input; it stops two seconds after the last honk.
void CommunicationAgent::shutOffHorn(Information* information)
{
    if ((information->getObjectTypeFlags() & 0x04000000u) == 0)
        return;
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    if (now - m_lastHonkTime > 2.0)
        m_bot->getPlayerInput()->setInput(io::PIFire, false);
}

bool CommunicationAgent::peekRadioMessage(CAPrioMessage& message)
{
    if (m_queue.empty())
        return false;
    message = m_queue.top();
    return true;
}

bool CommunicationAgent::dequeueRadioMessage(CAPrioMessage& message)
{
    if (m_queue.empty())
        return false;
    message = m_queue.top();
    m_queuedTypes.erase(message.m_type);
    m_queue.pop();
    return true;
}

// Waits until the message's time has come and the last one heard of its type has had its say.
bool CommunicationAgent::waitWithSendingMessage(const CAPrioMessage& message, double time)
{
    if (message.m_time > time)
        return true;
    const std::list<AIRadioMessageInfo>& messages = getReceivedMessages(message.m_type);
    if (messages.empty())
        return false;
    const AIRadioMessageInfo& last = messages.front();
    return time - last.m_double8 < static_cast<double>(m_maxSendPeriod[last.m_type]);
}

void CommunicationAgent::postEvent(const CAPrioMessage& message)
{
    const unsigned char playerId = static_cast<unsigned char>(m_bot->getPlayerId());
    const unsigned char side = static_cast<unsigned char>(m_bot->getSide());
    const unsigned char value = static_cast<unsigned char>(message.m_int12);
    int voice;
    if (static_cast<IBFModuleCourier*>(CommonSingletonServer::m_iBFModuleCourier)
            ->getVoiceFromAiMessageType(message.m_type, voice))
    {
        m_hub->filterAndSendAIRadioMessage(m_bot, playerId, static_cast<ChannelMessageType>(3),
                                           message.m_channel, message.m_type,
                                           static_cast<unsigned char>(voice), playerId, side, value);
    }
}

void CommunicationAgent::receiveRadioMessage(const AIRadioMessageSettings& settings, int senderId,
                                             bool self)
{
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    AIRadioMessageInfo message;
    message.m_float0 = settings.m_modifier;
    message.m_senderPlayerId = senderId;
    message.m_double8 = now;
    message.m_bool16 = false;
    message.m_bool17 = self;
    message.m_type = settings.m_type;

    if (settings.m_type == 0 || settings.m_type == 1)
    {
        std::list<AIRadioMessageInfo>& opposite = m_receivedMessages[1 - settings.m_type];
        for (std::list<AIRadioMessageInfo>::iterator it = opposite.begin(); it != opposite.end(); ++it)
        {
            if (it->m_senderPlayerId == senderId)
            {
                opposite.erase(it, opposite.end());
                break;
            }
        }
    }

    std::list<AIRadioMessageInfo>& inbox = m_receivedMessages[settings.m_type];
    for (std::list<AIRadioMessageInfo>::iterator it = inbox.begin(); it != inbox.end(); ++it)
    {
        if (it->m_senderPlayerId == senderId)
        {
            it->m_float0 = settings.m_modifier;
            it->m_bool16 = false;
            it->m_double8 = now;
            return;
        }
    }
    inbox.push_front(message);
}

// 0x1d5460 (Ghidra). The hub may report the sighting.
void CommunicationAgent::setSeenNewEnemy(Information* information)
{
    const int side = m_bot->getSide();
    IAIObject* const object = information->getAIObject();
    m_hub->enemySpottedByAI(m_bot->getPlayer(), object, side);
}

} // namespace ai
} // namespace hfe
} // namespace dice
