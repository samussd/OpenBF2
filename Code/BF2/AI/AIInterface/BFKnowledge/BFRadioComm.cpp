// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFRadioComm operations.

#include "BFRadioComm.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../../../Core/EventManager.h"
#include "../../../Game/GameEvents/RadioMessageEvent.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IBFRadioComm::IBFRadioComm()
{
    if (!CommonSingletonServer::m_iBFRadioComm)
        classManager->registerSimpleSingleton(std::string("IBFRadioComm"), this);
}

IBFRadioComm::~IBFRadioComm()
{
    if (CommonSingletonServer::m_iBFRadioComm == this)
        classManager->unregisterSimpleSingleton(std::string("IBFRadioComm"));
}

BFRadioComm::BFRadioComm()
{
}

BFRadioComm::~BFRadioComm()
{
}

// The AI's only way to speak: a local radio delivery, on the engine's channel for the AI channel.
int BFRadioComm::sendAIRadioMessage(int playerId, ChannelMessageType channel, AIRadioChannelType type,
                                    unsigned char voice, int senderId, unsigned char side,
                                    unsigned char value)
{
    static const int channels[] = { 6, 7, 8, 3, 2, 4, 10, 11, 14 };
    const unsigned int index = static_cast<unsigned int>(type);
    const int target = index < 9 ? channels[index] : 0x13;
    EventNode* const node = createRadioMessageReceivedEvent(playerId, voice,
                                                            static_cast<unsigned char>(senderId),
                                                            side, value, target,
                                                            static_cast<int>(channel));
    eventManager->postEvent(ECNetwork, node, 0.0f);
    return 0;   // [CHOSEN, not recovered] the caller discards it
}

} // namespace ai
} // namespace hfe
} // namespace dice
