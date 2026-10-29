#include "BBMoveTo.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/BotMemory.h"
#include "../AIBot/SquadCoordinator.h"
#include "../AIBot/SquadFormation.h"
#include "../AIBot/SquadManager.h"
#include "../AIBot/SquadMember.h"
#include "../AIBot/WaypointList.h"
#include "../AIBotBehaviourPattern/BBPGotoWaypoint.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BBMoveTo::BBMoveTo(unsigned char behaviourId, unsigned char mode, BBPGotoWaypoint* generator, UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
{
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_targets = 0;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
    }
}

BBMoveTo::~BBMoveTo()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BBMoveTo::~BBMoveTo");
}

// 0x229510 (Ghidra). How much the bot wants to follow its waypoints: the waypoint list's urgency
// where the bot is (times the memory's +8 factor while it obeys an order for this behaviour),
// 1000 while its squad member flees artillery the bot stands in, then scaled by the urgency
// handed in. A member at its formation slot whose squad is not moving out wants nothing. A
// positive urgency starts the plan, feeds the move inclination and hands the sensing agent the
// goal. A refresh without the full evaluation keeps the urgency only while the plan runs.
float BBMoveTo::calculateUrgency(Bot* bot, bool full, float value)
{
    SquadMember* const member = bot->getSquadMember();
    // no null guard, as shipped
    if (member->isAtFormationSlot()
        && !member->getSquad()->getSquadFormation()->shouldSquadMoveOut())
    {
        m_urgency[bot->getBotId()] = 0.0f;
        return 0.0f;
    }
    if (!full)
    {
        m_changedTarget[bot->getBotId()] = false;
        if (!m_generator->isInitiated(bot))
            m_urgency[bot->getBotId()] = 0.0f;
        bot->addToMoveInclination(m_urgency[bot->getBotId()]);
        return m_urgency[bot->getBotId()];
    }

    float scale = 1.0f;
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    if (bot->isOrderedBehaviour(m_behaviourId) && bot->isTakingOrders())
    {
        const unsigned int id = bot->getBotId();
        const float factor = bot->getMemory()->m_float8;
        const float urgency = bot->getWaypoints()->getUrgency(equipment->getPosition(), bot);
        m_urgency[id] = urgency * factor;
    }
    else
    {
        const float urgency = bot->getWaypoints()->getUrgency(equipment->getPosition(), bot);
        m_urgency[bot->getBotId()] = urgency;
    }

    const int side = bot->getSide();
    const Vec3 position = equipment->getPosition();
    double sinceCalled;
    int calledBy;
    if (SquadManager::m_instance->isInArtilleryDangerZone(position, side, sinceCalled, calledBy))
    {
        SquadMember* const fleeing = bot->getSquadMember();
        if (fleeing && fleeing->isAvoidingArtillery())
            m_urgency[bot->getBotId()] = 1000.0f;
    }
    scale *= value;
    m_urgency[bot->getBotId()] = scale * m_urgency[bot->getBotId()];
    if (m_urgency[bot->getBotId()] <= 0.0f)
    {
        const bool initiated = m_generator->isInitiated(bot);
        m_changedTarget[bot->getBotId()] = initiated;
        m_generator->unInit(bot);
        m_urgency[bot->getBotId()] = 0.0f;
        return 0.0f;
    }

    m_changedTarget[bot->getBotId()] = bot->getWaypoints()->m_targetChanged;
    static_cast<BBPGotoWaypoint*>(m_generator)->init(bot);
    bot->addToMoveInclination(m_urgency[bot->getBotId()]);
    Vec3 goal;
    goal.x = 0.0f;
    goal.y = 0.0f;
    goal.z = 0.0f;
    if (bot->getWaypoints()->getGoalPoint(goal))
        bot->insertBehaviourInformation(0.5f, goal, m_behaviourId);
    return m_urgency[bot->getBotId()];
}

} // namespace ai
} // namespace hfe
} // namespace dice
