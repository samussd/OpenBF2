
#include "BAPAUseFlareOnMissileLock.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../../IO/Input/PlayerInput.h"

namespace dice {
namespace hfe {
namespace ai {

BAPAUseFlareOnMissileLock::BAPAUseFlareOnMissileLock(Bot* bot, unsigned long long controls, int planId)
    : BAPAction(planId)
    , m_bot(bot)
    , m_lockTime(-1.0)
    , m_locked(false)
{
    m_controls = controls;
}

BAPAUseFlareOnMissileLock::~BAPAUseFlareOnMissileLock()
{
}

bool BAPAUseFlareOnMissileLock::execute(Bot* bot, bool& ok, bool& done)
{
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    IPIUnit* const unit = equipment->getUnit();
    const int lock = unit->getMissileLockStatus();
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    if (!m_locked && lock > 0)
    {
        m_locked = true;
        m_lockTime = now;
    }

    const double wait = (equipment->getObjectTypeFlags() & 0x10u) ? 1.0 : 0.0;
    if (m_locked && now - m_lockTime >= wait)
    {
        io::PlayerInput* const input = static_cast<BotMain*>(bot)->getPlayerInput();
        if ((input->mappingFlags >> io::PIFlareFire) & 1ULL)
            input->inputs[io::PIFlareFire] = 1.0f;
        unit->setMissileLockStatus(0);
        m_locked = false;
    }
    ok = true;
    done = true;
    return true;
}

BAPStatementType BAPAUseFlareOnMissileLock::getId() const
{
    return BAPSTUseFlareOnMissileLock;
}

bool BAPAUseFlareOnMissileLock::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
