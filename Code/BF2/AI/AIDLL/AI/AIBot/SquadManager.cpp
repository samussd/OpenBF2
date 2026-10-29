// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadManager operations.

#include "SquadManager.h"

#include <cmath>

#include "SquadCoordinator.h"                        // the constructor builds two per squad
#include "SquadMember.h"                             // addSquadMember moves a member between squads
#include "BotMain.h"
#include "BotSquad.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/IBotManager.h"
#include "../Main/AIMain.h"
#include "../Main/CommonSingletonServer.h"
#include "../AIStrategic/SAI.h"
#include "../AIStrategicEntity/AIStrategicArea.h"   // getClosestAvailableSquad
#include "../../../AIInterface/BFKnowledge/BFGame.h" // IAIGame::getMaxNumberOfSquads
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFSquad.h"
#include "../../../AIInterface/BFKnowledge/BFCommander.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/BFObjectEnvironment.h"  // friendliesCloseTo's player ids
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../Main/IAIPathfinding.h"                 // updateHumanPlayers' position check
#include "../Main/AIRandom.h"
#include "../../../../Core/SettingsRepostitory.h"
#include "../../../../Core/StringUtil.h"

namespace dice {
namespace hfe {
namespace ai {

// Both materialize the full integer sum.
// Side is one-based. Preserve the fresh vector bound and indexed receiver at every call.
// 0x1bfd30 (Ghidra). Distances are squared and compared in float.
SquadCoordinator* SquadManager::getClosestAvailableSquad(AIStrategicArea* area, const Vec3& position,
                                                         int side, bool attack, int& strength)
{
    SquadCoordinator* closest = 0;
    float best = 3.4028235e+38f;
    Vec3 squadPosition;
    squadPosition.x = 0.0f;
    squadPosition.y = 0.0f;
    squadPosition.z = 0.0f;
    strength = 0;
    std::vector<SquadCoordinator*>& squads = m_coordinators[side - 1];
    for (std::size_t i = 0; i < squads.size(); ++i)
    {
        SquadCoordinator* const squad = squads[i];
        if (!squad->isSquadAvailable() || squad->getNumberOfSquadMembers() <= 0)
            continue;
        if (squad->isSquadRequestedToArea(area->getIndex()) && !squad->getSquadLeader()->isBot())
            continue;
        const int squadStrength =
            squad->getStrategicStrength(static_cast<StrategicStrengthType>(!attack));
        if (squadStrength * 2 <= 0)
            continue;
        squad->getSquadPosition(squadPosition);
        const float distance = (position.x - squadPosition.x) * (position.x - squadPosition.x) +
                               (position.y - squadPosition.y) * (position.y - squadPosition.y) +
                               (position.z - squadPosition.z) * (position.z - squadPosition.z);
        if (distance < best)
        {
            strength = squadStrength * 2;
            closest = squad;
            best = distance;
        }
    }
    return closest;
}

// 0x1bfaa0 (Ghidra). From the leaders of the side's squads with members and a position;
// sqrt(FLT_MAX) when there is none.
float SquadManager::getDistanceToClosestSquad(const Vec3& position, int side)
{
    float best = 3.4028235e+38f;
    Vec3 leader;
    leader.x = 0.0f;
    leader.y = 0.0f;
    leader.z = 0.0f;
    const std::vector<SquadCoordinator*>& squads = m_coordinators[side - 1];
    for (std::size_t i = 0; i < squads.size(); ++i)
    {
        if (squads[i]->getNumberOfSquadMembers() <= 0)
            continue;
        if (!squads[i]->getSquadLeader()->getPosition(leader))
            continue;
        const float distance = (position.x - leader.x) * (position.x - leader.x) +
                               (position.y - leader.y) * (position.y - leader.y) +
                               (position.z - leader.z) * (position.z - leader.z);
        if (distance < best)
            best = distance;
    }
    return static_cast<float>(std::sqrt(static_cast<double>(best)));
}

int SquadManager::getSquadStrengths(int side, int areaIndex, StrategicStrengthType type)
{
    int total = 0;
    for (std::size_t index = 0; index < m_coordinators[side - 1].size(); ++index)
    {
        if (m_coordinators[side - 1][index]->getMissionStatus() == 2 &&
            m_coordinators[side - 1][index]->isSquadAssignedToArea(areaIndex))
        {
            total += m_coordinators[side - 1][index]->getStrategicStrength(type);
        }
    }
    return total;
}

SquadManager* SquadManager::m_instance = 0;

// One coordinator per call, alternating
// sides. Re-read both cursors and side-zero's size after the coordinator callback; no empty
// index or null guard exists. The size-minus-one comparison is unsigned, including underflow.
void SquadManager::update(double time)
{
    updateArtilleryStatus(time);
    updateCommanderMessages();
    m_sideCursor = m_sideCursor == 0;
    m_coordinators[m_sideCursor][m_squadCursor]->update(time);
    if (m_sideCursor == 1)
    {
        if (static_cast<unsigned int>(m_squadCursor) < m_coordinators[0].size() - 1)
            m_squadCursor = static_cast<int>(static_cast<unsigned int>(m_squadCursor) + 1);
        else
        {
            m_squadCursor = 0;
            updateHumanPlayers(time);
        }
    }
}

// Strict ordered expiry, then clear the flag before any
// path callback. Each expiry visits both sides, using live vector bounds/entries. Side one's
// flag, timestamp and duration are read after side zero's callbacks; simultaneous expiry
// therefore runs two complete traversals.
void SquadManager::updateArtilleryStatus(double time)
{
    SideBlock* blocks[2] = { &m_sideBlock0, &m_sideBlock1 };
    for (unsigned int block = 0; block < 2; ++block)
    {
        if (blocks[block]->m_b0 && time > m_double156 + blocks[block]->m_artilleryStartTime)
        {
            blocks[block]->m_b0 = false;
            for (unsigned int side = 0; side < 2; ++side)
                for (unsigned int index = 0; index < m_coordinators[side].size(); ++index)
                    m_coordinators[side][index]->updateSquadPaths();
        }
    }
}

// Purge messages for each human commander before reading
// game time once. Dispatch only types 0/1, but delete every expired type. The queued pointer
// is fetched again after the send callback for deletion, and the list end remains live.
void SquadManager::updateCommanderMessages()
{
    for (int side = 0; side < 2; ++side)
    {
        if (IAIMain::m_instance->getSAI(side + 1)->isPlayerCommander())
        {
            for (std::list<CommanderMessage*>::iterator i = m_commanderMessages.begin();
                 i != m_commanderMessages.end(); )
            {
                if ((*i)->m_sideIndex == side)
                {
                    delete *i;
                    m_commanderMessages.erase(i++);
                }
                else
                    ++i;
            }
        }
    }
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    for (std::list<CommanderMessage*>::iterator i = m_commanderMessages.begin();
         i != m_commanderMessages.end(); )
    {
        if (now > (*i)->m_dueTime)
        {
            switch (static_cast<int>((*i)->m_type))
            {
            case 0:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
                    ->sendAttackMessage((*i)->m_sideIndex + 1, (*i)->m_squadIndex + 1);
                break;
            case 1:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)
                    ->sendDefendMessage((*i)->m_sideIndex + 1, (*i)->m_squadIndex + 1);
                break;
            }
            delete *i;
            m_commanderMessages.erase(i++);
        }
        else
            ++i;
    }
}

// Keep the first invalid timestamp; repeated invalid
// reports do not refresh it. A valid report erases the entry, if any. One getId call.
void SquadManager::setPlayerOnValidPosition(IAIPlayer* player, bool valid, double time)
{
    const int id = player->getId();
    std::map<int, double>::iterator i = m_playerValidPositions.find(id);
    if (valid)
    {
        if (i != m_playerValidPositions.end())
            m_playerValidPositions.erase(i);
    }
    else if (i == m_playerValidPositions.end())
        m_playerValidPositions.insert(std::make_pair(id, time));
}

// Copy the borrowed factory
// vector before callbacks. Preserve the repeated AI-player query even when the first one
// returned true. A live, non-downed human stands on a valid position unless the record of
// the vehicle they are in (the soldier counts) is mobile and its position fails the
// pathfinding check for that vehicle's trace map and equipment type.
void SquadManager::updateHumanPlayers(double time)
{
    const std::vector<IAIPlayer*> players =
        static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)->getAllPlayers();
    for (std::vector<IAIPlayer*>::const_iterator i = players.begin(); i != players.end(); ++i)
    {
        IAIPlayer* player = *i;
        if (!player)
            continue;
        bool valid = true;
        if (!player->getIsAIPlayer() && player->getIsAlive() && !player->isManDown())
        {
            IAIObject* const vehicle = player->getCurrentVehicle();
            if (vehicle)
            {
                // [PLATFORM] ELF captures the manager before the getSide and getInformation calls.
                InformationManager* const manager = informationManager;
                Information* const information =
                    manager->getInformation(vehicle->getInformation(player->getSide()));
                if (information)
                {
                    const Vec3 position = information->getPosition();
                    IPIMobile* const mobile = information->getMobile();
                    if (mobile)
                    {
                        // Retail asks the equipment type before the vehicle number.
                        const int equipmentType = information->getUnit()->getEquipmentType();
                        const int vehicleNumber = mobile->getVehicleNumber();
                        if (!IAIPathfinding::instance->isValidPosition(vehicleNumber, equipmentType,
                                                                       position, false))
                            valid = false;
                    }
                }
            }
        }
        if (!player->getIsAIPlayer())
            setPlayerOnValidPosition(player, valid, time);
    }
    for (std::map<int, double>::iterator i = m_playerValidPositions.begin();
         i != m_playerValidPositions.end(); )
    {
        IAIPlayer* player = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
            ->getPlayerFromId(i->first);
        if (!player || player->getIsAIPlayer() || !player->getIsAlive() || player->isManDown())
            m_playerValidPositions.erase(i++);
        else
            ++i;
    }
}

// Creates a coordinator for each squad on each side, excluding squad zero.
SquadManager::SquadManager()
    : m_coordinators()
    // Default-initialize SideBlock: its three Vec3 values alone are zeroed. Value-initializing
    // the blocks would also zero timestamps/flags retail leaves untouched (full //PE reads).
    , m_commanderMessages()
    , m_playerValidPositions()
{
    const int squads =
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getMaxNumberOfSquads();

    if (squads != 1)
    {
        const unsigned int last = static_cast<unsigned int>(squads) - 1;

        for (unsigned int i = 0; i < last; ++i)
        {
            m_coordinators[0].push_back(new SquadCoordinator(0, static_cast<int>(i)));
            m_coordinators[1].push_back(new SquadCoordinator(1, static_cast<int>(i)));
        }
    }

    if (!m_instance)
        m_instance = this;

    m_squadCursor = 0;
    m_float152 = 40.0f;
    m_sideCursor = 1;

    m_sideBlock0.m_b0 = false;
    m_sideBlock1.m_b0 = false;

    m_double156 = 25.0;

    clearCommanderMessages();
    clearPlayerValidPositions();
}

// Deletes each owned commander message before erasing its list node.
// Advances the iterator before erasure.
void SquadManager::clearCommanderMessages()
{
    for (std::list<CommanderMessage*>::iterator i = m_commanderMessages.begin();
         i != m_commanderMessages.end(); )
    {
        delete *i;
        m_commanderMessages.erase(i++);
    }
}

// Erases position records individually, preserving per-node tree rebalancing.
void SquadManager::clearPlayerValidPositions()
{
    for (std::map<int, double>::iterator i = m_playerValidPositions.begin();
         i != m_playerValidPositions.end(); )
    {
        m_playerValidPositions.erase(i++);
    }
}

// complete instructions read on both ports. Prefer a nonempty
// least-populated coordinator, with later entries winning ties. Keep the repeated size reads.
// There is no side-range, empty-roster or player-null guard in either image.
void SquadManager::addSquadMember(int playerId, int side)
{
    --side;
    IAIPlayer* player = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
        ->getPlayerFromId(playerId);
    SquadCoordinator* selected = 0;
    const int maximum = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)
        ->getMaxNumberOfSquadMembers();
    int best = maximum;
    std::vector<SquadCoordinator*>& coordinators = m_coordinators[side];
    for (unsigned int i = 0; i < coordinators.size(); ++i)
    {
        if (coordinators[i]->getNumberOfSquadMembers() <= best &&
            coordinators[i]->getNumberOfSquadMembers() > 0)
        {
            best = coordinators[i]->getNumberOfSquadMembers();
            selected = coordinators[i];
        }
    }
    if (!selected)
        selected = coordinators[0];

    if (static_cast<unsigned int>(selected->getSquadId()) != coordinators.size() - 1 &&
        coordinators[selected->getSquadId() + 1]->getNumberOfSquadMembers() == 0)
    {
        int splitAt = 3;
        const std::string mode = settings->stringGet("GSGameMode", "gpm_cq");
        if (compareIgnoreCase(mode, std::string("gpm_coop")) == 0)
            splitAt = maximum - 1;
        best = selected->getNumberOfSquadMembers();
        if (best >= splitAt)
        {
            // The newest member moves to the empty next squad, which then takes the new player.
            // The engine sees the member leave and join; the old record is rebuilt and deleted.
            // The bot arm has no null check, as shipped.
            SquadMember* const moved = selected->getSquadMember(best - 1);
            selected->removeSquadMember(best - 1);
            selected->updateSquadLeader();
            static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad)
                ->exitSquad(selected->getSquadId() + 1, moved->getPlayer());
            selected = coordinators[selected->getSquadId() + 1];
            selected->addSquadMember(moved->getPlayer());
            if (!moved->isBot())
            {
                static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad)
                    ->joinSquad(selected->getSquadId() + 1, moved->getPlayer());
            }
            else
            {
                const int squad = selected->getSquadId() + 1;
                IBotManager::instance->getBotFromPlayerId(moved->getPlayerId())
                    ->getBotSquad()->joinSquad(squad);
            }
            selected->updateSquadLeader();
            delete moved;
        }
    }

    selected->addSquadMember(player);
    Bot* bot = IBotManager::instance->getBotFromPlayerId(playerId);
    if (bot)
    {
        const int squad = selected->getSquadId() + 1;
        bot->getBotSquad()->joinSquad(squad);
    }
    else
    {
        static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad)
            ->joinSquad(selected->getSquadId() + 1, player);
    }
    // Both shipped callers discard the bridge's Boolean; a refusal still reaches this update.
    selected->updateSquadLeader();
}

// Both identifiers are one-based. Retail has no range
// singleton or coordinator-null check here; the courier owns only the singleton guard.
void SquadManager::newSquadLeader(int side, int squadId)
{
    m_coordinators[side - 1][squadId - 1]->updateSquadLeader();
}

// Search this side in coordinator order and return the first
// matching member. The coordinator's output index is intentionally not used by this caller.
// 0x1bf480 (Ghidra).
SquadMember* SquadManager::getSquadMember(unsigned int handle, int side)
{
    Information* const information = informationManager->getInformation(handle);
    if (!information || !information->getUnit())
        return 0;
    const int playerId = information->getUnit()->getOccupyingPlayerId(side);
    if (playerId == -1)
        return 0;
    return getSquadMember(playerId, side);
}

// 0x1bf600 (Ghidra).
SquadCoordinator* SquadManager::getSquadFromPlayer(unsigned int handle, int side)
{
    Information* const information = informationManager->getInformation(handle);
    if (!information || !information->getUnit())
        return 0;
    const int playerId = information->getUnit()->getOccupyingPlayerId(side);
    if (playerId == -1)
        return 0;
    return getSquadFromPlayer(playerId, side);
}

// 0x1be5b0 (Ghidra). The player list is copied first.
bool SquadManager::haveHumanPlayerOnOwnSide(int side)
{
    const std::vector<IAIPlayer*> players =
        static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)->getAllPlayers();
    for (std::vector<IAIPlayer*>::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        if (*it && !(*it)->getIsAIPlayer() && (*it)->getSide() == side)
            return true;
    }
    return false;
}

// 0x1c0440 (Ghidra).
bool SquadManager::isAnySquadMemberRespondingToRequest(unsigned int handle, int action, int side)
{
    std::vector<SquadCoordinator*>& coordinators = m_coordinators[side - 1];
    for (unsigned int i = 0; i < coordinators.size(); ++i)
    {
        if (coordinators[i]->isAnySquadMemberRespondingToRequest(handle, action))
            return true;
    }
    return false;
}

// 0x1c0390 (Ghidra).
float SquadManager::requestResponseModifier(SquadMember* member, unsigned int handle, int action,
                                            int side)
{
    SquadCoordinator* const squad = getSquadFromPlayer(member->getPlayerId(), side);
    if (!squad)
        return 0.0f;
    if (member->isSameRequest(handle, action))
        return 1.0f;
    return squad->requestResponseModifier(member, handle, action);
}

SquadMember* SquadManager::getSquadMember(int playerId, int side)
{
    std::vector<SquadCoordinator*>& coordinators = m_coordinators[side - 1];
    for (unsigned int i = 0; i < coordinators.size(); ++i)
    {
        int memberIndex;
        SquadMember* member = coordinators[i]->getSquadMember(playerId, memberIndex);
        if (member)
            return member;
    }
    return 0;
}

// The side is one-based and unchecked. Keep the live
// unsigned-index scan and reload the coordinator entry after a successful member query.
// The member index output is not consumed, and null coordinator entries are not guarded.
SquadCoordinator* SquadManager::getSquadFromPlayer(int playerId, int side)
{
    std::vector<SquadCoordinator*>& coordinators = m_coordinators[side - 1];
    for (unsigned int i = 0; i < coordinators.size(); ++i)
    {
        int memberIndex;
        if (coordinators[i]->getSquadMember(playerId, memberIndex))
            return coordinators[i];
    }
    return 0;
}

// Squad zero is ignored. An existing member anywhere on the
// side also returns immediately: a bot's engine join notification must not insert it twice.
// On a miss, retain the unchecked coordinator/player lookups, add, then update the leader.
void SquadManager::playerJoinsSquad(int playerId, int squadId, int side)
{
    if (squadId == 0)
        return;
    if (getSquadMember(playerId, side))
        return;

    SquadCoordinator* coordinator = m_coordinators[side - 1][squadId - 1];
    IAIPlayer* player = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
        ->getPlayerFromId(playerId);
    coordinator->addSquadMember(player);
    coordinator->updateSquadLeader();
}

void SquadManager::reset()
{
    unAssignSquads(1);
    unAssignSquads(2);
    clearCommanderMessages();
    clearPlayerValidPositions();
    m_sideBlock0.m_b0 = false;
    m_sideBlock1.m_b0 = false;
    m_squadCursor = 0;
    m_sideCursor = 1;
    for (unsigned int i = 0; i < m_coordinators[0].size(); ++i)
        m_coordinators[0][i]->reset();
    for (unsigned int i = 0; i < m_coordinators[1].size(); ++i)
        m_coordinators[1][i]->reset();
}

// One-based side, unchecked.
void SquadManager::unAssignSquads(int side)
{
    for (unsigned int i = 0; i < m_coordinators[side - 1].size(); ++i)
        m_coordinators[side - 1][i]->unAssignSquad();
}

int SquadManager::unAssignSquads(int areaIndex, int side)
{
    int released = 0;
    for (unsigned int i = 0; i < m_coordinators[side - 1].size(); ++i)
    {
        SquadCoordinator* const squad = m_coordinators[side - 1][i];
        if (!squad->isSquadCloseToNonOwnedTarget() && squad->isSquadAssignedToArea(areaIndex))
        {
            squad->unAssignSquad();
            ++released;
        }
    }
    return released;
}

float SquadManager::getArtillerySafeDistance()
{
    return m_float152;
}

namespace {

// The record of the vehicle the player is in, as the side sees it; null when there is none.
Information* vehicleInformation(IAIPlayer* player, int side)
{
    IAIObject* const vehicle = player->getCurrentVehicle();
    if (!vehicle)
        return 0;
    return informationManager->getInformation(vehicle->getInformation(side));
}

} // namespace

// Any player of the side within the distance of the point, in three dimensions. Only records
// with type flag 0x20, or without flag 0x10, count.
bool SquadManager::friendliesCloseTo(Vec3 position, float distance, int side)
{
    std::vector<int> playerIds;
    static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment)
        ->getWorldPlayerIds(playerIds);
    IAIPlayerFactory* const players =
        static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory);
    for (std::vector<int>::iterator it = playerIds.begin(); it != playerIds.end(); ++it)
    {
        IAIPlayer* const player = players->getPlayerFromId(*it);
        if (!player || player->getSide() != side)
            continue;
        Information* const information = vehicleInformation(player, side);
        if (!information)
            continue;
        const unsigned int flags = information->getObjectTypeFlags();
        if (!(flags & 0x20) && (flags & 0x10))
            continue;
        const Vec3& at = information->getPosition();
        const float dx = at.x - position.x;
        const float dy = at.y - position.y;
        const float dz = at.z - position.z;
        if (dx * dx + dy * dy + dz * dz < distance * distance)
            return true;
    }
    return false;
}

// The first enemy of the side, in player-id order, who has held an invalid position for more
// than a minute; its XZ position is written out. The record is taken as the enemy sees itself.
bool SquadManager::haveCampingHumans(int side, Vec2& position)
{
    const int enemy = side == 1 ? 2 : 1;
    const double now =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    IAIPlayerFactory* const players =
        static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory);
    for (std::map<int, double>::iterator it = m_playerValidPositions.begin();
         it != m_playerValidPositions.end(); ++it)
    {
        IAIPlayer* const player = players->getPlayerFromId(it->first);
        if (!player || player->getSide() != enemy || !(it->second + 60.0 < now))
            continue;
        Information* const information = vehicleInformation(player, player->getSide());
        if (!information)
            continue;
        const Vec3& at = information->getPosition();
        position.x = at.x;
        position.y = at.z;
        return true;
    }
    return false;
}

// Warns the side of a strike at the position: the side's block records when and where, and a
// delay of half the configured time scaled by one plus a random fraction. Every flee position,
// on both sides, becomes invalid.
// 0x1c0b00 (Ghidra).
bool SquadManager::isInArtilleryDangerZone(Vec3 position, int side, double& timeSince, int& bySide)
{
    SideBlock* const blocks[2] = { &m_sideBlock0, &m_sideBlock1 };
    const int other = side - 1 == 0 ? 1 : 0;
    timeSince = -1.0f;
    bySide = 0;
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    const float safeSq = m_float152 * m_float152;

    const SideBlock& own = *blocks[side - 1];
    if (own.m_b0)
    {
        const float dx = own.m_artilleryPosition.x - position.x;
        const float dy = own.m_artilleryPosition.y - position.y;
        const float dz = own.m_artilleryPosition.z - position.z;
        if (safeSq > dx * dx + dy * dy + dz * dz)
        {
            timeSince = now - own.m_artilleryStartTime;
            bySide = side;
            return true;
        }
    }
    const SideBlock& enemy = *blocks[other];
    if (enemy.m_b0)
    {
        const float dx = enemy.m_artilleryPosition.x - position.x;
        const float dy = enemy.m_artilleryPosition.y - position.y;
        const float dz = enemy.m_artilleryPosition.z - position.z;
        if (safeSq > dx * dx + dy * dy + dz * dz
            && now > enemy.m_artilleryStartTime + enemy.m_artilleryDelay)
        {
            timeSince = now - enemy.m_artilleryStartTime;
            bySide = other + 1;
            return true;
        }
    }
    return false;
}

void SquadManager::incomingArtillery(Vec3 position, int side)
{
    SideBlock* const blocks[2] = { &m_sideBlock0, &m_sideBlock1 };
    const float halfDelay = 0.5f * static_cast<float>(m_double156);
    SideBlock& block = *blocks[side - 1];
    block.m_b0 = true;
    block.m_artilleryStartTime =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    block.m_artilleryPosition = position;
    const float fraction = AIRandom::m_instance->nextFloat();
    block.m_validFlee0 = false;
    block.m_validFlee1 = false;
    block.m_artilleryDelay = halfDelay + fraction * halfDelay;
    SideBlock& other = *blocks[side - 1 == 0 ? 1 : 0];
    other.m_validFlee0 = false;
    other.m_validFlee1 = false;
}

// Squad zero is ignored, and so is a player the side does not know. Only the squad the engine
// names is searched, so a member held by another squad stays. The member record is not deleted,
// as shipped: a member the AI moved itself was already removed before the engine reported it.
void SquadManager::playerRemovedFromSquad(int playerId, int squadId, int side)
{
    if (squadId == 0)
        return;
    SquadMember* const member = getSquadMember(playerId, side);
    if (!member)
        return;

    SquadCoordinator* coordinator = m_coordinators[side - 1][squadId - 1];
    coordinator->removeSquadMember(member);
    coordinator->updateSquadLeader();
}

} // namespace ai
} // namespace hfe
} // namespace dice
