
#include "BAPASendRadioMessage.h"

#include "../AIBot/BotMain.h"

namespace dice {
namespace hfe {
namespace ai {

BAPASendRadioMessage::BAPASendRadioMessage(AIRadioMessageType type, AIRadioChannelType channel,
                                           unsigned int priority, float delay, int planId,
                                           int value)
    : BAPAction(planId)
    , m_type(type)
    , m_channel(channel)
    , m_priority(priority)
    , m_value(value)
    , m_delay(delay)
{
}

BAPASendRadioMessage::~BAPASendRadioMessage()
{
}

bool BAPASendRadioMessage::execute(Bot* bot, bool& ok, bool& done)
{
    bot->getCommunicationAgent()->enqueueRadioMessage(m_type, m_channel, m_priority, m_delay, m_value);
    ok = true;
    done = true;
    return true;
}

BAPStatementType BAPASendRadioMessage::getId() const
{
    return BAPSTSendRadioMessage;
}

bool BAPASendRadioMessage::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
