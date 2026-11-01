#include "BAPIWFire.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../Main/IBotManager.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x206d50 (Ghidra).
BAPIWFire::BAPIWFire(Bot* bot, unsigned int target, bool b1, bool b2, int i1, int i2, BAPStatement* statement,
                     int planId)
    : BAPInfoWrapper(statement, planId)
    , m_target(target)
    , m_int1C(i1)
    , m_int20(i2)
    , m_bool24(b1)
    , m_bool25(b2)
    , m_int28(-1)
    , m_bot(bot)
{
    m_statement->m_owner = this;
    bot->setIsFiring(true);
    bot->setFiringTarget(target);
    m_useStaticAttack = static_cast<BotMain*>(bot)->getUseStaticAttack();
}

// 0x206ef0 (Ghidra).
BAPIWFire::~BAPIWFire()
{
    if (m_statement)
        delete m_statement;
    BotMain* const bot = static_cast<BotMain*>(m_bot);
    bot->setIsFiring(false);
    bot->setBridgeRepairTarget(static_cast<unsigned int>(-1));
    bot->setBotIsSimulatingArtillery(false);
    bot->setRepairTarget(static_cast<unsigned int>(-1));
    bot->setIsGoingForFlag(false);
    bot->setFiringTarget(static_cast<unsigned int>(-1));
    if (m_target == static_cast<unsigned int>(-1))
        return;
    Information* const target = informationManager->getInformation(m_target);
    if (!target)
        return;
    if (Bot* const targetBot = IBotManager::instance->getBotFromPlayerId(target->getPlayerId()))
        static_cast<BotMain*>(targetBot)->setIsEngineerTarget(false);
}

int BAPIWFire::getInfoType() const
{
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
