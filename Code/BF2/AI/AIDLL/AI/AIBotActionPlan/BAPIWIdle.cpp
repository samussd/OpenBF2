#include "BAPIWIdle.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/SquadMember.h"

namespace dice {
namespace hfe {
namespace ai {

// Retail does not guard the statement.
BAPIWIdle::BAPIWIdle(const Vec3& position, BAPStatement* statement, int planId, Bot* bot)
    : BAPInfoWrapper(statement, planId)
    , m_position(position)
    , m_bot(bot)
{
    if (m_bot && m_bot->getSquadMember())
        m_bot->getSquadMember()->setIsIdle(true);
    m_statement->m_owner = this;
}

BAPIWIdle::~BAPIWIdle()
{
    if (m_bot && m_bot->getSquadMember())
        m_bot->getSquadMember()->setIsIdle(false);
    if (m_statement)
        delete m_statement;
}

int BAPIWIdle::getInfoType() const
{
    return 4;
}

} // namespace ai
} // namespace hfe
} // namespace dice
