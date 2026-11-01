// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadCoordinator operations.

#include "SquadCoordinator.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../Main/AIRandom.h"
#include "../Main/AISettingsInterfaces.h"
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"

#include "SquadFormation.h"
#include "SquadMember.h"
#include "BotMain.h"
#include "CommunicationAgent.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../Main/IAIPathfinding.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../Main/IBotManager.h"
#include "SquadManager.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AIMain.h"
#include "../AIStrategic/SAI.h"
#include "../AIStrategicEntity/AIStrategicArea.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"      // IBFTimer::getGameTime
#include "../../../AIInterface/BFKnowledge/BFSquad.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"

namespace dice {
namespace hfe {
namespace ai {

// [INFERRED] local helper name. PE's Vec3 subtraction helper spills all three
// deltas to float, then its length-squared helper keeps (dx*dx+dy*dy)+dz*dz wide
// through the ordered comparison. ELF keeps the subtractions wide and adds
// (dy*dy+dx*dx)+dz*dz. Only the MSVC x86 branch is runtime-validated here.
static bool maneuverMemberWithinRange(const Vec3& position, const Vec3& squadPosition)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const member = &position;
    const Vec3* const squad = &squadPosition;
    const float threshold = 8100.0f;
    float deltaX, deltaY, deltaZ;
    bool within = false;
    __asm {
        mov eax, squad
        mov edx, member
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fstp deltaY
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fstp deltaZ
        fld deltaX
        fmul st(0), st(0)
        fld deltaY
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld deltaZ
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp threshold
        fnstsw ax
        test ah, 5
        jp notWithin
        mov within, 1
    notWithin:
    }
    return within;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(squadPosition.x) - position.x);
    const long double dy = static_cast<float>(static_cast<long double>(squadPosition.y) - position.y);
    const long double dz = static_cast<float>(static_cast<long double>(squadPosition.z) - position.z);
#else
    const long double dx = static_cast<long double>(squadPosition.x) - position.x;
    const long double dy = static_cast<long double>(squadPosition.y) - position.y;
    const long double dz = static_cast<long double>(squadPosition.z) - position.z;
#endif
#if defined(_WIN32)
    return (dx * dx + dy * dy) + dz * dz < 8100.0L;
#else
    return (dy * dy + dx * dx) + dz * dz < 8100.0L;
#endif
#endif
}

// [INFERRED] local helper name. A shooter's squared distance from where it first fired, ordered
// strictly above 25 (five metres). PE spills only dx to float and sums (dz*dz+dy*dy)+dx*dx;
// ELF keeps all three differences wide and sums (dy*dy+dx*dx)+dz*dz. Equality/NaN keep the record.
static bool shooterMovedAway(const Vec3& firePosition, const Vec3& position)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const fire = &firePosition;
    const Vec3* const member = &position;
    const float threshold = 25.0f;
    float deltaX;
    bool moved = false;
    __asm {
        mov eax, fire
        mov edx, member
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st(0), st(0)
        fxch st(1)
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld deltaX
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp threshold
        fnstsw ax
        test ah, 41h
        jnz notMoved
        mov moved, 1
    notMoved:
    }
    return moved;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(firePosition.x) - position.x);
#else
    const long double dx = static_cast<long double>(firePosition.x) - position.x;
#endif
    const long double dy = static_cast<long double>(firePosition.y) - position.y;
    const long double dz = static_cast<long double>(firePosition.z) - position.z;
#if defined(_WIN32)
    return (dz * dz + dy * dy) + dx * dx > 25.0L;
#else
    return (dy * dy + dx * dx) + dz * dz > 25.0L;
#endif
#endif
}

// [INFERRED] local helper name. The leader's squared distance to the goto position, narrowed to
// float. PE subtracts through its float-returning Vec3 operator (position - goto); ELF keeps
// goto - position wide. Both sum (dx*dx+dy*dy)+dz*dz.
static float leaderGotoDistanceSq(const Vec3& position, const Vec3& gotoPosition)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const p = &position;
    const Vec3* const g = &gotoPosition;
    float deltaX, deltaY, deltaZ, distance;
    __asm {
        mov eax, p
        mov edx, g
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fstp deltaY
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fstp deltaZ
        fld deltaX
        fmul st(0), st(0)
        fld deltaY
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld deltaZ
        fmul st(0), st(0)
        faddp st(1), st(0)
        fstp distance
    }
    return distance;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(position.x) - gotoPosition.x);
    const long double dy = static_cast<float>(static_cast<long double>(position.y) - gotoPosition.y);
    const long double dz = static_cast<float>(static_cast<long double>(position.z) - gotoPosition.z);
#else
    const long double dx = static_cast<long double>(gotoPosition.x) - position.x;
    const long double dy = static_cast<long double>(gotoPosition.y) - position.y;
    const long double dz = static_cast<long double>(gotoPosition.z) - position.z;
#endif
    return static_cast<float>((dx * dx + dy * dy) + dz * dz);
#endif
}

// [INFERRED] local helper name. Squared member distance ordered strictly below the (float)
// squared radius. PE: control point - position through its float Vec3 operator, then
// (dx*dx+dy*dy)+dz*dz; ELF: position - control point wide, then (dy*dy+dx*dx)+dz*dz.
static bool insideControlPointRadius(const Vec3& controlPoint, const Vec3& position, float radiusSq)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const c = &controlPoint;
    const Vec3* const p = &position;
    float deltaX, deltaY, deltaZ;
    bool inside = false;
    __asm {
        mov eax, c
        mov edx, p
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fstp deltaY
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fstp deltaZ
        fld deltaX
        fmul st(0), st(0)
        fld deltaY
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld deltaZ
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp radiusSq
        fnstsw ax
        test ah, 5
        jp outside
        mov inside, 1
    outside:
    }
    return inside;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(controlPoint.x) - position.x);
    const long double dy = static_cast<float>(static_cast<long double>(controlPoint.y) - position.y);
    const long double dz = static_cast<float>(static_cast<long double>(controlPoint.z) - position.z);
    return (dx * dx + dy * dy) + dz * dz < static_cast<long double>(radiusSq);
#else
    const long double dx = static_cast<long double>(position.x) - controlPoint.x;
    const long double dy = static_cast<long double>(position.y) - controlPoint.y;
    const long double dz = static_cast<long double>(position.z) - controlPoint.z;
    return (dy * dy + dx * dx) + dz * dz < static_cast<long double>(radiusSq);
#endif
#endif
}

// [INFERRED]
// void return: the manager discards it and exits do not materialize a common result.
// Timestamp and mission-2 assignment checks precede even the empty-member return.
void SquadCoordinator::update(double time)
{
    m_d124 = time;
    if (m_i120 == 2 && !IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
    {
        if (m_i40 == 0)
        {
            if (m_strategicAreaAssignment &&
                m_strategicAreaAssignment->getStatus(m_side + 1) != 0)
            {
                if (!m_strategicAreaAssignment->isAreaTakeable())
                {
                    // Both the main singleton and side are fresh after the area callback;
                    // the second side read is later still, after getSAI has returned.
                    SAI* const sai = IAIMain::m_instance->getSAI(m_side + 1);
                    if (!sai->ownAllTakeableCps(m_side + 1))
                        unAssignSquad();
                }
            }
            else if (m_strategicAreaAssignment &&
                     m_strategicAreaAssignment->getStatus(m_side + 1) == 0)
            {
                unAssignSquad();
            }
        }
        else if (m_i40 == 1 && m_strategicAreaAssignment &&
                 m_strategicAreaAssignment->getStatus(m_side + 1) == 0)
        {
            // Opposite one-based side is captured before getSAI; it is not reloaded
            // after that callback, unlike the ownAllTakeableCps argument above.
            const int otherSide = (m_side == 0) + 1;
            if (IAIMain::m_instance->getSAI(m_side + 1)->haveAnyOwnedAndTakeableCps(otherSide))
                unAssignSquad();
        }
    }

    if (m_squadMembers.empty())
        return;

    // Both images reload the member vector after each call, without a null-member guard.
    for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
        m_squadMembers[index]->updateFormationStatus();
    m_formation->updateClosestSlots();

    // The second live loop forwards the original double, not the mutable stored time.
    // It is skipped if the preceding callbacks emptied the vector; maneuver still follows.
    for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
        m_squadMembers[index]->update(time);

    updateSquadManeuver();

    m_formation->update(time);
    updateSquadPosition();
    updateShotsFired();
    // Each timed tail receives the original double argument, not the stored m_d124.
    updateRepairRequest(time);
    updateNeedOrdersMessage(time);
    updateIncomingMessages();
}

// [INFERRED] Updates the squad position from its living anchor, otherwise its first
// living member. Retains the old position when nobody is alive. Anchor/leader indices
// must be valid; lookups are unchecked and position-validity results are ignored.
void SquadCoordinator::updateSquadPosition()
{
    Vec3 position = {};
    const int anchorIndex = m_formation->getSquadMemberIndexAtSlot(0);
    SquadMember* member = anchorIndex == -1 ? getSquadLeader() : m_squadMembers[anchorIndex];
    if (!member->getPlayer()->getIsAlive())
    {
        member = 0;
        for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
        {
            if (m_squadMembers[index]->getPlayer()->getIsAlive())
            {
                member = m_squadMembers[index];
                break;
            }
        }
    }
    if (member)
        member->getPosition(position);
    else
        position = m_v108;
    m_v108 = position;
}

// [INFERRED] Removes fire records with invalid shooter positions. Expired records or
// shooters more than five metres away also clear member paths and the movement flag.
// Reuses the position local, deletes records before nodes, and re-reads the list end.
void SquadCoordinator::updateShotsFired()
{
    Vec3 position = {};
    for (std::list<SquadMemberFire*>::iterator it = m_list12.begin(); it != m_list12.end(); )
    {
        SquadMemberFire* const fire = *it;
        if (!fire->m_member->getPosition(position))
        {
            delete *it;
            m_list12.erase(it++);
        }
        else if (m_d124 > fire->m_time || shooterMovedAway(fire->m_position, position))
        {
            delete *it;
            m_list12.erase(it++);
            // Not updateSquadPaths: this inline walk has no null-member test.
            for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
                m_squadMembers[index]->setHavePath(false);
            m_formation->updateMovementFlag(true);
        }
        else
            ++it;
    }
}

// Strict ordered expiry clears the request and
// its confirmation together; the position and deadline are left as they were.
void SquadCoordinator::updateRepairRequest(double time)
{
    if (m_b172 && time > m_d188)
    {
        m_b172 = false;
        m_b196 = false;
    }
}

// Requests orders for an idle bot squad leader after twenty seconds without orders.
void SquadCoordinator::updateNeedOrdersMessage(double time)
{
    if (IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander() && !isSquadTakingFlag())
    {
        const int orderState = m_i40;
        if (orderState == 0 || orderState == 2 || orderState == 4)
        {
            SquadMember* const leader = getSquadLeader();
            float distanceSq = 1000.0f;
            Vec3 position = {};
            if (leader)
            {
                if (leader->getPosition(position))
                    distanceSq = leaderGotoDistanceSq(position, m_v28);
                if (leader->isBot() && !leader->isMoving() &&
                    leader->getPlayer()->getIsAlive() && !leader->getPlayer()->isManDown() &&
                    (distanceSq < 225.0f ||
                     (m_strategicAreaAssignment &&
                      m_strategicAreaAssignment->getStatus(m_side + 1) == 0)))
                {
                    // The one path that keeps the running timer.
                    if (!(time > m_needOrderMessageTimer + 20.0))
                        return;
                    static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad)
                        ->requestNewCommanderOrder(m_squadIndex + 1, leader->getPlayer());
                    leader->getBot()->getCommunicationAgent()->enqueueRadioMessage(
                        static_cast<AIRadioMessageType>(14), static_cast<AIRadioChannelType>(4),
                        3, 1.0f, 0);
                }
            }
        }
    }
    resetNeedOrderMessageTimer(time);
}

// Routes a human leader's call to one bot when the formation has left the leader.
void SquadCoordinator::updateIncomingMessages()
{
    SquadMember* const leader = getSquadLeader();
    if (!leader || leader->isBot())
        return;
    if (!m_formation->shouldLeaveSquadLeader())
        return;

    bool found = false;
    SquadMember* member = 0;
    for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
    {
        member = m_squadMembers[index];
        if (member->isBot() && member->getPlayer()->getIsAlive())
        {
            found = true;
            break;
        }
    }
    if (!found)
        return;

    CommunicationAgent* const radio = member->getBot()->getCommunicationAgent();
    const std::list<AIRadioMessageInfo>& messages =
        radio->getReceivedMessages(static_cast<AIRadioMessageType>(12));
    if (messages.empty())
        return;
    if (messages.front().m_senderPlayerId != leader->getPlayerId())
        return;

    m_formation->joinSquadLeader();
    radio->enqueueRadioMessage(static_cast<AIRadioMessageType>(6),
                               static_cast<AIRadioChannelType>(3), 3, 2.0f, 0);
    updateSquadPaths();
}

// Returns whether a living, standing squad member is inside the assigned control point
// for an area this side does not own. Requires a valid member position and a successful
// control-point query; squares the radius in float.
bool SquadCoordinator::isSquadTakingFlag()
{
    bool taking = false;
    if (m_strategicAreaAssignment && m_strategicAreaAssignment->getStatus(m_side + 1) != 0)
    {
        Vec3 controlPoint = {};
        Vec3 position = {};
        float radius;
        if (m_strategicAreaAssignment->getControlPointPosAndRadius(controlPoint, radius))
        {
            radius = radius * radius;
            for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
            {
                SquadMember* const member = m_squadMembers[index];
                if (member->getPlayer()->getIsAlive() && !member->getPlayer()->isManDown() &&
                    member->getPosition(position) &&
                    insideControlPointRadius(controlPoint, position, radius))
                {
                    taking = true;
                    break;
                }
            }
        }
    }
    return taking;
}

// [INFERRED] void result: discarded by update, no common return materialization.
void SquadCoordinator::updateSquadManeuver()
{
    m_i148 = chooseSquadManeuver();
    switch (m_i148)
    {
    case 0:
        for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
        {
            SquadMember* const member = m_squadMembers[index];
            if (member->isBot())
                member->getBot()->setAllowFire(true);
        }
        break;
    case 1:
        // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
        BF2_AI_FRONTIER("dice::hfe::ai::SquadCoordinator::updateSquadManeuver (squadManeuverAlternateFire)");
    case 2:
        squadManeuverStaticFlank();
        break;
    }
}

// The squad may fire. Once per mission, 65% of the time, the leader takes the area's nearest
// flanking route, if the area has one its vehicle can use.
void SquadCoordinator::squadManeuverStaticFlank()
{
    for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
    {
        SquadMember* const member = m_squadMembers[index];
        if (member->isBot())
            member->getBot()->setAllowFire(true);
    }

    if (m_b157 || m_i120 != 2 || !m_formation->hasSquadMembers() || !m_strategicAreaAssignment)
        return;
    m_b157 = true;
    if (!(AIRandom::m_instance->nextFloat() < 0.65f))
        return;

    SquadMember* const leader = m_squadMembers[m_formation->getSquadMemberIndexAtSlot(0)];
    if (m_strategicAreaAssignment->getClosestFlankingPositions(m_v108, m_positions[0], m_positions[1])
        && isValidPosition(leader, m_positions[0]) && isValidPosition(leader, m_positions[1]))
    {
        m_b156 = true;
        leader->setHavePath(false);
    }
}

// Return carries the full integer selector; the source enum name remains unknown.
int SquadCoordinator::chooseSquadManeuver()
{
    int firingMembers = 0;
    Vec3 position = {};
    const unsigned int count = static_cast<unsigned int>(m_squadMembers.size());
    for (unsigned int index = 0; index < count; ++index)
    {
        SquadMember* const member = m_squadMembers[index];
        if (member->isBot() && member->getPlayer()->getIsAlive())
        {
            Bot* const bot = member->getBot();
            // [PLATFORM] ELF captures the manager before the equipment callback.
            // PE loads it afterward, and only when the low handle word is nonzero.
#if defined(_WIN32)
            const unsigned int handle = bot->getCurrentEquipment();
            Information* const information = (handle & 0xffffu)
                ? informationManager->getInformation(handle) : nullptr;
#else
            InformationManager* const manager = informationManager;
            const unsigned int handle = bot->getCurrentEquipment();
            Information* const information = manager->getInformation(handle);
#endif
            if (bot->isFiring())
            {
                // No information-null guard exists in either image.
                const unsigned int flags = information->getObjectTypeFlags();
                if (!(flags & 0x10u) && !(flags & 0x20u) && !(flags & 0x80u) &&
                    member->getPosition(position) &&
                    maneuverMemberWithinRange(position, m_v108))
                    ++firingMembers;
            }
        }
    }
    if (firingMembers > 1)
        return 1;
    if (m_i120 != 2)
        return 0;

    // Flank a commander's target from between 50 and about 173 metres, while the area is hostile.
    const float x = m_v28.x;
    const float z = m_v28.z;
    const float dy = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain)
                         ->getTerrainAltitude(x, z) - m_v108.y;
    const float dx = x - m_v108.x;
    const float dz = z - m_v108.z;
    const float distanceSq = dx * dx + dy * dy + dz * dz;
    if (distanceSq < 30000.0f && distanceSq > 2500.0f && m_b56 && m_strategicAreaAssignment
        && m_strategicAreaAssignment->getStatus(m_side + 1) == 1)
    {
        if (m_i148 != 2)
        {
            m_b156 = false;
            m_b157 = false;
        }
        return 2;
    }
    return 0;
}

// [INFERRED] void return, as above.
// Null entries are skipped; a live unsigned index walk invalidates each real member.
void SquadCoordinator::updateSquadPaths()
{
    for (std::size_t index = 0; index < m_squadMembers.size(); ++index)
    {
        SquadMember* const member = m_squadMembers[index];
        if (member)
            member->setHavePath(false);
    }
}

// Borrowed formation pointer, unchanged.
SquadFormation* SquadCoordinator::getSquadFormation()
{
    return m_formation;
}

// Return the address of the stored
// destination. [INFERRED] const-reference spelling; callers read the borrowed Vec3.
const Vec3& SquadCoordinator::getSquadGotoPos()
{
    return m_v28;
}

// Full-width field read; the original
// return enum spelling is unresolved, so int records the observed payload without new labels.
int SquadCoordinator::getMissionStatus()
{
    return m_i120;
}

// The assignment is borrowed and nullable.
bool SquadCoordinator::isSquadAssignedToArea(int areaIndex)
{
    return m_strategicAreaAssignment && m_strategicAreaAssignment->getIndex() == areaIndex;
}

// 0x1b7fa0 (Ghidra).
bool SquadCoordinator::isSquadAvailable()
{
    return m_i120 == 0;
}

// 0x1b7f60 (Ghidra).
bool SquadCoordinator::isSquadRequestedToArea(int areaIndex)
{
    return m_requestedArea && m_requestedArea->getIndex() == areaIndex;
}

// Each qualifying
// live member needs an IAIObject information-handle query before any strength can be added.
int SquadCoordinator::getStrategicStrength(StrategicStrengthType type)
{
    float total = 0.0f;
    for (std::size_t index = 0; index < m_squadMembers.size(); ++index)
    {
        IAIPlayer* const player = m_squadMembers[index]->getPlayer();
        if (!player->getIsAlive() || player->isManDown())
            continue;
        IAIObject* const vehicle = player->getCurrentVehicle();
        if (!vehicle)
            continue;

        Information* const information =
            informationManager->getInformation(vehicle->getInformation(m_side + 1));
        if (information && information->getUnit())
            total += information->getUnit()->getStrategicStrengths()[type];
    }
    return static_cast<int>(total + 0.5f);
}

// Stores side and squad identity before constructing the formation, which queries this coordinator.

SquadCoordinator::SquadCoordinator(int side, int squadIndex)
    : m_squadMembers()
    , m_list12()
    , m_squadIndex(squadIndex)
    , m_side(side)
    , m_strategicAreaAssignment(0)
    , m_i40(4)
    , m_i44(0)
    , m_d48(-20.0)
    , m_requestedArea(0)
    , m_i76(4)
    , m_i80(0)
    , m_i96(-1)
    , m_list104()
    , m_i120(0)
    , m_d124(0.0)
    , m_d132(0.0)
    , m_d140(0.0)
    , m_i148(0)
    , m_i152(0)
    , m_b156(false)
    , m_b157(false)
    , m_positions()
    , m_b172(false)
    , m_b196(false)
    , m_b220(false)
{
    m_v28.x = 0.0f;
    m_v28.y = 0.0f;
    m_v28.z = 0.0f;
    m_v64.x = 0.0f;
    m_v64.y = 0.0f;
    m_v64.z = 0.0f;
    m_v108.x = 0.0f;
    m_v108.y = 0.0f;
    m_v108.z = 0.0f;
    m_v176.x = 0.0f;
    m_v176.y = 0.0f;
    m_v176.z = 0.0f;
    m_v200.x = 0.0f;
    m_v200.y = 0.0f;
    m_v200.z = 0.0f;

    m_formation = new SquadFormation(this);

    m_v108.x = 0.0f;
    m_v108.y = 0.0f;
    m_v108.z = 0.0f;

    // two of them, pushed one after the other. See the header.
    Vec3 zero;
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    m_positions.push_back(zero);
    m_positions.push_back(zero);

    m_v200.x = 0.0f;
    m_v200.y = 0.0f;
    m_v200.z = 0.0f;

    resetNeedOrderMessageTimer(
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime());

    m_b220 = false;
}

int SquadCoordinator::getSide()
{
    return m_side;
}

// Copy the stored squad position
// (retail ELF +0x6C / PE +0x8C), without testing squadHasPosition. [INFERRED]
// void return: both callers discard it; EAX holds different scratch values per port.
void SquadCoordinator::getSquadPosition(Vec3& position)
{
    position = m_v108;
}

// The stored leader index is
// used directly, including the absence of an empty-vector or negative-index guard.
SquadMember* SquadCoordinator::getSquadLeader()
{
    return m_squadMembers[m_i96];
}

// Signed count consumed by the manager's comparisons.
int SquadCoordinator::getNumberOfSquadMembers()
{
    return static_cast<int>(m_squadMembers.size());
}

// non-const in the mangled signature.
int SquadCoordinator::getSquadId()
{
    return m_squadIndex;
}

// Zero-distance test with 0.001f, including true for NaN.
// PE sums z squared, y squared, then x squared. No square root or component-wise epsilon.
bool SquadCoordinator::squadHasPosition()
{
    const float x = -m_v108.x;
    const float y = -m_v108.y;
    const float z = -m_v108.z;
    return !(z * z + y * y + x * x < 0.001f);
}

// PE inlined at : four stores in this order.
void SquadCoordinator::unAssignSquad()
{
    m_strategicAreaAssignment = 0;
    m_i40 = 4;
    m_i44 = 0;
    m_i120 = 0;
}

// Allocation is not checkPointer-wrapped. The human arm
// queries the one-based side's commander before publishing the member in the vector.
void SquadCoordinator::addSquadMember(IAIPlayer* player)
{
    SquadMember* member = new SquadMember(this, player, m_squadIndex, m_side,
                                         static_cast<int>(m_squadMembers.size()));
    if (!member->isBot())
    {
        if (!IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
            unAssignSquad();
    }
    m_squadMembers.push_back(member);
    if (member->isAllowedInFormation())
    {
        m_formation->joinFormation(member->getMemberIndex());
        m_formation->updateMovementFlag(true);
    }
    member->updateMemberPos();
}

// Stop on the first engine-recognized leader; later members
// are not visited. No found leader leaves the old index intact. A first leader skips the
// change arm, but the single-member trailing-position reset still runs. A changed leader
// moves the formation and squad paths to it; a human leader of two or more members loses the
// squad's area assignment unless a player commands the side.
void SquadCoordinator::updateSquadLeader()
{
    const int oldLeader = m_i96;
    bool found = false;
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        m_squadMembers[i]->setIsSquadLeader(
            static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad)->isSquadLeader(
                m_squadIndex + 1, m_squadMembers[i]->getPlayer()));
        if (m_squadMembers[i]->isSquadLeader())
        {
            m_i96 = static_cast<int>(i);
            found = true;
            break;
        }
    }
    if (!found)
        return;

    if (oldLeader != m_i96 && oldLeader != -1 && !m_squadMembers.empty())
    {
        m_squadMembers[m_i96]->updateMemberPos();
        m_formation->joinSquadLeader();
        updateSquadPosition();
        updateSquadPaths();
        if (getSquadLeader() && !getSquadLeader()->isBot() && m_squadMembers.size() > 1
            && !IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
        {
            unAssignSquad();
        }
    }
    if (m_squadMembers.size() == 1)
        m_formation->resetTrailingPositions();
}

// The member leaves the formation before the vector erase; the leader update and the slot
// reassignment then see the renumbered roster. The member itself is not deleted.
void SquadCoordinator::removeSquadMember(int index)
{
    SquadMember* const member = m_squadMembers[index];
    removeSquadMemberFire(member);
    if (m_formation->isInFormation(member->getMemberIndex()))
        m_formation->leaveFormation(member->getMemberIndex());
    m_squadMembers.erase(m_squadMembers.begin() + index);
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
        m_squadMembers[i]->setMemberIndex(static_cast<int>(i));
    updateSquadLeader();
    m_formation->reassignSlots();
}

void SquadCoordinator::destroySquadMember(int index)
{
    SquadMember* const member = m_squadMembers[index];
    removeSquadMemberFire(member);
    member->removeRequest();
    if (m_formation->isInFormation(member->getMemberIndex()))
        m_formation->leaveFormation(member->getMemberIndex());
    m_squadMembers.erase(m_squadMembers.begin() + index);
    if (member->isBot())
        member->getBot()->setSquadMember(0);
    delete member;
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
        m_squadMembers[i]->setMemberIndex(static_cast<int>(i));
    updateSquadLeader();
    m_formation->reassignSlots();
}

// Matches by player id, first match only; an unknown member is ignored.
void SquadCoordinator::removeSquadMember(SquadMember* member)
{
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        if (member->getPlayerId() == m_squadMembers[i]->getPlayerId())
        {
            removeSquadMember(static_cast<int>(i));
            return;
        }
    }
}

// Every record of the member's player id is deleted, not just the first.
void SquadCoordinator::removeSquadMemberFire(SquadMember* member)
{
    const int playerId = member->getPlayer()->getId();
    std::list<SquadMemberFire*>::iterator it = m_list12.begin();
    while (it != m_list12.end())
    {
        if ((*it)->m_playerId == playerId)
        {
            delete *it;
            it = m_list12.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// - one indexed load through the vector's data pointer.
// no bounds check. Same shape as `BotManager::getBot`, and the same reason to leave it alone.
SquadMember* SquadCoordinator::getSquadMember(int index)
{
    return m_squadMembers[index];
}

// First matching player wins. The output index is written
// only on success, and the vector/member is fetched again after the player-id call.
SquadMember* SquadCoordinator::getSquadMember(int playerId, int& index)
{
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        if (m_squadMembers[i]->getPlayerId() == playerId)
        {
            index = static_cast<int>(i);
            return m_squadMembers[i];
        }
    }
    return 0;
}

// Skip null members, but do not guard their player.
// Compare the complete returned integer; no alive/bot/leader filter exists. Both images
// refresh the member vector after a nonmatching virtual getSpawnKit callback.
bool SquadCoordinator::hasMemberWithKit(KitType type)
{
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        if (m_squadMembers[i] && m_squadMembers[i]->getPlayer()->getSpawnKit() == type)
            return true;
    }
    return false;
}

// 0x1b49f0 (Ghidra).
bool SquadCoordinator::isRepairRequested()
{
    return m_b172;
}

// 0x1b4a00 (Ghidra).
const Vec3& SquadCoordinator::getRepairRequestPos()
{
    return m_v176;
}

// 0x1b6b60 (Ghidra). No null guards on the vehicle, the soldier or their information, as shipped.
bool SquadCoordinator::isValidPosition(SquadMember* member, const Vec3& position)
{
    IAIPathfinding* const pathfinding = IAIPathfinding::instance;
    Information* information = informationManager->getInformation(
        member->getPlayer()->getCurrentVehicle()->getInformation(m_side + 1));
    IPIMobile* mobile = information->getMobile();
    unsigned int polygon;
    unsigned int quadTree;
    if (!mobile)
    {
        information = informationManager->getInformation(
            member->getPlayer()->getSoldier()->getInformation(m_side + 1));
        mobile = information->getMobile();
        const IPIMobile::ValidPos last = mobile->getLastValidPosition();
        if (!pathfinding->getPolygon(position, mobile->getVehicleNumber(), polygon, quadTree,
                                     information->getUnit()->getEquipmentType()))
            return false;
        if (last.quadTree == static_cast<unsigned int>(-1) || quadTree != last.quadTree)
            return false;
        quadTree = last.quadTree;
    }
    else
    {
        mobile->getLastValidPosition();         // copied and not read
        if (!pathfinding->getPolygon(position, mobile->getVehicleNumber(), polygon, quadTree,
                                     information->getUnit()->getEquipmentType()))
            return false;
    }
    return pathfinding->isValidPosition(mobile->getVehicleNumber(),
                                        information->getUnit()->getEquipmentType(), position,
                                        quadTree, true);
}

// 0x1b86d0 (Ghidra).
bool SquadCoordinator::isAnySquadMemberRespondingToRequest(unsigned int handle, int action)
{
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        if (m_squadMembers[i]->isSameRequest(handle, action))
            return true;
    }
    return false;
}

namespace {

// A living requester: its health above 0.001. No null guard on the information, as shipped.
bool isAlive(unsigned int handle)
{
    IPIPhysical* const physical = informationManager->getInformation(handle)->getPhysical();
    return physical && physical->getHealth() > 0.001f;
}

} // namespace

// 0x1b8730 (Ghidra). Squad sides are zero-based here, so the manager calls add one.
float SquadCoordinator::requestResponseModifier(SquadMember* member, unsigned int handle, int action)
{
    float modifier = 1.0f;
    bool answered = false;
    Vec3 requesterPosition;
    requesterPosition.x = 0.0f;
    requesterPosition.y = 0.0f;
    requesterPosition.z = 0.0f;
    SquadManager* const manager = SquadManager::m_instance;
    SquadMember* const requester = manager->getSquadMember(handle, m_side + 1);

    if (requester && requester->getSquadId() == member->getSquadId())
    {
        if (requester->getMemberIndex() != member->getMemberIndex() && action == 1 && isAlive(handle)
            && requester->getPlayer()->getSpawnKit() == 5)
        {
            modifier = 0.0f;
        }
        else
        {
            if (action == 4 && requester->getMemberIndex() != member->getMemberIndex()
                && isAlive(handle) && requester->getPlayer()->getSpawnKit() == 3)
            {
                modifier = 0.0f;
                answered = true;
            }
            if (!answered)
            {
                for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
                {
                    SquadMember* const other = m_squadMembers[i];
                    if (member->getMemberIndex() == other->getMemberIndex()
                        || !other->isSameRequest(handle, action))
                        continue;
                    if (action != 5)
                    {
                        modifier = 0.0f;
                        break;
                    }
                    modifier *= 0.75f;
                }
            }
        }
    }
    else if (requester)
    {
        const bool responding = manager->isAnySquadMemberRespondingToRequest(handle, action, m_side + 1);
        float distanceSquared = 0.0f;
        if (requester->getPosition(requesterPosition))
        {
            const float dx = requesterPosition.x - m_v108.x;
            const float dy = requesterPosition.y - m_v108.y;
            const float dz = requesterPosition.z - m_v108.z;
            distanceSquared = dx * dx + dy * dy + dz * dz;
        }
        int kit = -1;
        if (action == 1)
            kit = 5;
        else if (action == 2)
            kit = 4;
        else if (action == 4)
            kit = 3;
        if (kit != -1)
        {
            if (responding || !(1000.0f > distanceSquared))
            {
                modifier = 0.0f;
            }
            else
            {
                SquadCoordinator* const squad =
                    manager->getSquadFromPlayer(requester->getPlayerId(), m_side + 1);
                modifier = squad->hasMemberWithKit(static_cast<KitType>(kit)) ? 0.1f : 0.5f;
            }
        }
    }
    else if (manager->isAnySquadMemberRespondingToRequest(handle, action, m_side + 1))
    {
        modifier = 0.0f;
    }
    else
    {
        Information* const information = informationManager->getInformation(handle);
        modifier = 1.0f;
        if (information)
        {
            const Vec3& position = information->getPosition();
            const float dx = position.x - m_v108.x;
            const float dy = position.y - m_v108.y;
            const float dz = position.z - m_v108.z;
            if (dx * dx + dy * dy + dz * dz > 20000.0f)
                modifier = 0.0f;
            IPIUnit* const unit = information->getUnit();
            if (unit)
            {
                const int playerId = unit->getOccupyingPlayerId(m_side + 1);
                if (playerId != -1 && !IBotManager::instance->getBotFromPlayerId(playerId))
                    modifier = modifier + modifier;
            }
        }
    }

    if (requester && !requester->isBot())
        modifier = modifier + modifier;
    return modifier;
}

// Only pointer presence is tested;
// there is no area validity, membership, side or mission-state query.
bool SquadCoordinator::isSquadAssignedToAnyArea()
{
    return m_strategicAreaAssignment != 0;
}

// Return the stored pointer unchanged.
// ELF callers use it directly as AIStrategicArea::getRouteList/orderNormalBot's this;
// the paired kit callers read its area index at +0xF0. No ownership is transferred.
AIStrategicArea* SquadCoordinator::getSquadStrategicAreaAssignment()
{
    return m_strategicAreaAssignment;
}

// - one store of a double.
// Drops the area assignment, maneuver, repair request and shot records, rejoins the formation
// on the leader, restarts the order-request timer and resets every member.
void SquadCoordinator::reset()
{
    m_strategicAreaAssignment = 0;
    m_i40 = 4;
    m_d48 = -20.0;
    m_i44 = 0;
    m_i76 = 4;
    m_requestedArea = 0;
    m_i80 = 0;
    m_i120 = 0;
    m_formation->joinSquadLeader();
    for (std::list<SquadMemberFire*>::iterator it = m_list12.begin(); it != m_list12.end();)
    {
        delete *it;
        it = m_list12.erase(it);
    }
    m_formation->reassignSlots();
    const Vec3 zero = { 0.0f, 0.0f, 0.0f };
    m_v108 = zero;
    m_i148 = 0;
    m_i152 = 0;
    m_d132 = 0.0;
    m_d140 = 0.0;
    m_b156 = false;
    m_b157 = false;
    m_b172 = false;
    m_b196 = false;
    m_v200 = zero;
    resetNeedOrderMessageTimer(
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime());
    m_b220 = false;
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
        m_squadMembers[i]->reset();
}

bool SquadCoordinator::squadHasMission()
{
    return m_i120 == 2;
}

// The first member close to the goal answers; members after it are not asked.
bool SquadCoordinator::isSquadCloseToNonOwnedTarget()
{
    if (!m_strategicAreaAssignment
        || static_cast<int>(m_strategicAreaAssignment->getStatus(m_side + 1)) == 0)
    {
        return false;
    }
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        if (m_squadMembers[i]->isCloseToNonOwnedTarget())
            return true;
    }
    return false;
}

void SquadCoordinator::resetNeedOrderMessageTimer(double now)
{
    m_needOrderMessageTimer = now;
}

const Vec3& SquadCoordinator::getSquadLeaderGotoPos()
{
    return m_v200;
}

bool SquadCoordinator::getRogerThatPositionMessage()
{
    return m_b220;
}

void SquadCoordinator::setRogerThatPositionMessage(bool answer)
{
    m_b220 = answer;
}

// 0x1b4dc0 (Ghidra). No vehicle counts as water; no null guards on its record, as shipped.
bool SquadCoordinator::isPosOnWater(SquadMember* member, const Vec3& position)
{
    IAIObject* const vehicle = member->getPlayer()->getCurrentVehicle();
    if (!vehicle)
        return true;
    Information* const information = informationManager->getInformation(vehicle->getInformation(m_side + 1));
    const int type = information->getUnit()->getEquipmentType();
    const int map = information->getMobile()->getVehicleNumber();
    unsigned char material = 0;
    IAIPathfinding::instance->getMaterial(position, map, type, material);
    return material == 4;
}

// 0x1b6b20 (Ghidra).
bool SquadCoordinator::getSquadMissionTargetPos(Vec3& position)
{
    if (m_i44 != 0)
        return false;
    position = m_v28;
    return true;
}

// 0x1b79c0 (Ghidra).
bool SquadCoordinator::getRandomValidPosition(SquadMember* member, Vec3& position, float radius)
{
    if (isValidPosition(member, position))
        return true;
    const IAITerrain* const terrain = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
    for (int i = 0; i < 8; ++i)
    {
        Vec3 point;
        float draw = AIRandom::m_instance->nextFloat();
        point.x = (draw + draw) * radius + (position.x - radius);
        draw = AIRandom::m_instance->nextFloat();
        point.z = (draw + draw) * radius + (position.z - radius);
        point.y = terrain->getTerrainAltitude(point.x, point.z);
        if (isValidPosition(member, point))
        {
            position = point;
            return true;
        }
    }
    return false;
}

// 0x1b7bc0 (Ghidra).
bool SquadCoordinator::getValidSaPos(SquadMember* member, Vec3& position)
{
    AIStrategicObject* const closest = AIStrategicObjectManager::getInstance()->getClosestObject(m_v28, m_side + 1);
    if (!member || !closest)
        return false;
    IAIObject* const vehicle = member->getPlayer()->getCurrentVehicle();
    if (!vehicle)
        return false;
    Information* const information = informationManager->getInformation(vehicle->getInformation(m_side + 1));
    if (!information || !information->getUnit() || !information->getMobile())
        return false;
    position = closest->getOrderPos(information->getMobile()->getVehicleNumber(), m_side + 1);
    return true;
}

// 0x1b7d30 (Ghidra).
bool SquadCoordinator::getValidSaPos(SquadMember* member, AIStrategicArea* area, Vec3& position)
{
    if (!member || !area)
        return false;
    IAIObject* const vehicle = member->getPlayer()->getCurrentVehicle();
    if (!vehicle)
        return false;
    Information* const information = informationManager->getInformation(vehicle->getInformation(m_side + 1));
    if (!information || !information->getUnit() || !information->getMobile())
        return false;
    position = area->getOrderPos(information->getMobile()->getVehicleNumber(), m_side + 1);
    return true;
}

// 0x1b4950 (Ghidra).
void SquadCoordinator::requestRepair(const Vec3& position)
{
    m_b172 = true;
    m_v176 = position;
    m_b196 = false;
    m_d188 = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime() + 60.0f;
}

// 0x1b69c0 (Ghidra).
bool SquadCoordinator::isSameMission(int type, const Vec3& position, int target)
{
    return m_i40 == type && m_i44 == target && std::fabs(m_v28.x - position.x) <= 1.1920929e-07f
        && std::fabs(m_v28.y - position.y) <= 1.1920929e-07f && std::fabs(m_v28.z - position.z) <= 1.1920929e-07f;
}

// 0x1b6f30 (Ghidra). A bot leader's own order goes nowhere. Near a target the side does not own, or
// with a request pending, an AI commander's order is dropped. The request stands for 10 s.
void SquadCoordinator::requestMission(int type, const Vec3& position, int target, bool fromCommander)
{
    SquadMember* const leader = getSquadLeader();
    if (!fromCommander)
    {
        if (m_squadMembers.empty())
            return;
        const bool same = std::fabs(position.x - m_v200.x) <= 1.1920929e-07f
            && std::fabs(position.y - m_v200.y) <= 1.1920929e-07f
            && std::fabs(position.z - m_v200.z) <= 1.1920929e-07f;
        if (!same && !leader->isBot())
        {
            m_b220 = true;
            m_v200 = position;
            updateSquadPaths();
            m_formation->leaveSquadLeader();
        }
        return;
    }
    if (((!IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander() && isSquadCloseToNonOwnedTarget())
         || m_i120 != 0)
        && !IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
        return;
    m_i120 = 1;
    m_d132 = 10.0f + static_cast<float>(m_d124);
    m_v64 = position;
    m_i76 = type;
    m_requestedArea = 0;
    m_i80 = target;
    m_b92 = fromCommander;
}

// 0x1b7160 (Ghidra). Takes the requested mission to the strategic object closest to its position
// when the position is inside it or within the ignore distance past its size and the side holds it,
// otherwise to a valid point near the position. The leader says so on the radio, or a commander
// message goes out for a human one, and every member looks for a new path.
bool SquadCoordinator::acceptMission(SquadMember* member)
{
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    bool accepted = false;
    m_strategicAreaAssignment = 0;
    AIStrategicArea* const closest =
        static_cast<AIStrategicArea*>(AIStrategicObjectManager::getInstance()->getClosestObject(m_v64, m_side + 1));
    if (!IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander() && closest && m_i76 == 0
        && closest->getStatus(m_side + 1) == 0)
    {
        unAssignSquad();
        return false;
    }
    if (!IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander() && m_i40 == m_i76
        && m_strategicAreaAssignment == closest)
        return true;
    if (closest)
    {
        const Vec2& flat = closest->getPosition(m_side + 1);
        Vec3 at;
        at.x = flat.x;
        at.z = flat.y;
        at.y = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain)->getTerrainAltitude(at.x, at.z);
        const float ignore = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getIgnoreCommanderDistance();
        const float size = closest->m_size[m_side];
        bool take = true;
        if (!closest->isInside(m_v64))
        {
            const float x = m_v64.x - at.x;
            const float y = m_v64.y - at.y;
            const float z = m_v64.z - at.z;
            if (ignore + size <= std::sqrt(x * x + y * y + z * z) || closest->getStatus(m_side + 1) == 0)
                take = false;
        }
        if (take)
        {
            m_v64 = at;
            m_v28 = at;
            m_i120 = 2;
            m_i40 = m_i76;
            m_strategicAreaAssignment = closest;
            m_i44 = m_i80;
            m_d48 = now;
            m_b56 = m_b92;
            accepted = true;
        }
    }
    IBFSquad* const adapter = static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad);
    if (!m_strategicAreaAssignment)
    {
        Vec3 point = m_v64;
        if (getRandomValidPosition(member, point, 30.0f))
        {
            m_v64 = point;
            m_i120 = 2;
            m_v28 = point;
            m_i40 = m_i76;
            m_i44 = m_i80;
            m_b56 = m_b92;
            m_d48 = now;
            accepted = true;
        }
        else if (member->isBot())
        {
            member->getBot()->getCommunicationAgent()->enqueueRadioMessage(
                static_cast<AIRadioMessageType>(7), static_cast<AIRadioChannelType>(4), 3, 2.0f, 0);
            if (member->isSquadLeader())
            {
                if (m_i76 == 0)
                    adapter->declineCommanderAttackOrder(m_squadIndex + 1, member->getPlayer());
                else if (m_i76 == 1)
                    adapter->declineCommanderDefendOrder(m_squadIndex + 1, member->getPlayer());
                else if (m_i76 == 2)
                    adapter->declineCommanderMoveOrder(m_squadIndex + 1, member->getPlayer());
            }
            unAssignSquad();
            updateSquadPaths();
        }
    }
    if (!accepted)
        return false;
    SquadMember* const leader = getSquadLeader();
    if (!leader->isBot())
    {
        if (!IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander() && (m_i40 == 0 || m_i40 == 1))
            SquadManager::m_instance->enqueueCommanderMessage(m_side, m_squadIndex, m_i40 == 0 ? 0 : 1, 3.0f);
    }
    else
    {
        CommunicationAgent* const agent = leader->getBot()->getCommunicationAgent();
        if (IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
            agent->enqueueRadioMessage(static_cast<AIRadioMessageType>(6), static_cast<AIRadioChannelType>(4), 3,
                                       2.5f, 0);
        int message = -1;
        if (m_i40 == 0)
        {
            adapter->issueSquadAttackOrder(m_v28, m_squadIndex + 1, m_side + 1, 0);
            adapter->acceptCommanderAttackOrder(m_squadIndex + 1, member->getPlayer());
            message = 0x10;
        }
        else if (m_i40 == 1)
        {
            adapter->issueSquadDefendOrder(m_v28, m_squadIndex + 1, m_side + 1, 0);
            adapter->acceptCommanderDefendOrder(m_squadIndex + 1, member->getPlayer());
            message = 0xF;
        }
        else if (m_i40 == 2)
        {
            adapter->issueSquadMoveOrder(m_v28, m_squadIndex + 1, m_side + 1, 0);
            adapter->acceptCommanderMoveOrder(m_squadIndex + 1, member->getPlayer());
            message = 0x11;
        }
        if (message != -1)
            agent->enqueueRadioMessage(static_cast<AIRadioMessageType>(message), static_cast<AIRadioChannelType>(3), 3,
                                       2.0f, 0);
    }
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
        m_squadMembers[i]->setHavePath(false);
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
