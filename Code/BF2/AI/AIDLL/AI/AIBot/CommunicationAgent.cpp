// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining CommunicationAgent operations.

#include "CommunicationAgent.h"

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

// [FRONTIER] Remaining message operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

// [FRONTIER] Services one bot's radio on the BotManager communication tick.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when radio update behavior is recovered.
void CommunicationAgent::update()
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationAgent::update");
}

} // namespace ai
} // namespace hfe
} // namespace dice
