// Puts a radio message in the bot's queue and finishes at once.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASENDRADIOMESSAGE_H
#define AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASENDRADIOMESSAGE_H

#include "BAPAction.h"
#include "../AIBot/CommunicationAgent.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPASendRadioMessage : public BAPAction
{
public:
    BAPASendRadioMessage(AIRadioMessageType type, AIRadioChannelType channel, unsigned int priority,
                         float delay, int planId, int value);
    virtual ~BAPASendRadioMessage();

    virtual bool execute(Bot* bot, bool& ok, bool& done);
    virtual BAPStatementType getId() const;
    virtual bool hasDefault();

private:
    /* +0x14 */ AIRadioMessageType m_type;
    /* +0x18 */ AIRadioChannelType m_channel;
    /* +0x1C */ unsigned int m_priority;
    /* +0x20 */ int m_value;
    /* +0x24 */ float m_delay;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTACTIONPLAN_BAPASENDRADIOMESSAGE_H
