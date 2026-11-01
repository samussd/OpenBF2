// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadMember operations.

#include "SquadMember.h"

#include "../Main/AIDllFrontier.h"
#include "../Main/AIMain.h"
#include "../Main/AIRandom.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/IBotManager.h"
#include "BotMain.h"
#include "CommunicationAgent.h"
#include "SquadCoordinator.h"
#include "SquadFormation.h"
#include "SquadManager.h"
#include "WPAltitudeMoveTo.h"
#include "WPMoveTo.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../AIStrategic/SAI.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../AIUtils/checkPointer.h"
#include "../../../AIInterface/BFKnowledge/BFSquad.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"
#include "../../../../IO/Input/PlayerInput.h"
#include "../AIStrategicEntity/AIStrategicArea.h"   // isCloseToNonOwnedTarget: area status
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"

namespace dice {
namespace hfe {
namespace ai {

// [INFERRED] local helper names. The update pair compares the x87 sum directly;
// spilling previous+9 to double first would change the boundary at large times.
static bool memberPathUpdateDue(double previous, double now)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const double interval = 9.0;
    bool due = false;
    __asm {
        fld previous
        fadd interval
        fcomp now
        fnstsw ax
        test ah, 5
        jp notDue
        mov due, 1
    notDue:
    }
    return due;
#else
    return static_cast<long double>(now) > static_cast<long double>(previous) + 9.0L;
#endif
}

// [PLATFORM] PE spills only dx, then sums (dz*dz+dx*dx)+dy*dy. ELF retains
// all three deltas wide and sums (dx*dx+dy*dy)+dz*dz. Compare before narrowing.
// MSVC x86 is the validated target; the portable long-double arm is not an ELF
// runtime or x64-equivalence claim.
static bool memberPathDisplaced(const Vec3& previous, const Vec3& current)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const oldPosition = &previous;
    const Vec3* const newPosition = &current;
    const float threshold = 0.01f;
    float deltaX;
    bool displaced = false;
    __asm {
        mov eax, oldPosition
        mov edx, newPosition
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st(0), st(0)
        fld deltaX
        fmul st(0), st(0)
        faddp st(1), st(0)
        fxch st(1)
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp threshold
        fnstsw ax
        test ah, 41h
        jnz notDisplaced
        mov displaced, 1
    notDisplaced:
    }
    return displaced;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(previous.x) - current.x);
#else
    const long double dx = static_cast<long double>(previous.x) - current.x;
#endif
    const long double dy = static_cast<long double>(previous.y) - current.y;
    const long double dz = static_cast<long double>(previous.z) - current.z;
#if defined(_WIN32)
    return (dz * dz + dx * dx) + dy * dy > static_cast<long double>(0.01f);
#else
    return (dx * dx + dy * dy) + dz * dz > static_cast<long double>(0.01f);
#endif
#endif
}

// Preserve the stores around the two virtual calls: publishing
// this member to its bot happens before request/position flags are initialized. The Vec3 members
// are cleared before the body and again later. There is no null player/manager guard.
SquadMember::SquadMember(SquadCoordinator* squad, IAIPlayer* player,
                         int squadIndex, int side, int memberIndex)
    : m_position{0.0f, 0.0f, 0.0f}
    , m_vector54{0.0f, 0.0f, 0.0f}
    , m_vector60{0.0f, 0.0f, 0.0f}
{
    m_player = player;
    m_playerId = m_player->getId();
    m_squadIndex = squadIndex;
    m_isSquadLeader = false;
    m_memberIndex = memberIndex;
    m_squad = squad;
    m_maxSpeed = 0.0f;
    m_side = side;
    m_estimatedArrival = 1.0f;

    Bot* const bot = IBotManager::instance->getBotFromPlayerId(m_playerId);
    if (bot)
    {
        m_bot = bot;
        m_bot->setSquadMember(this);
    }
    else
    {
        m_bot = 0;
    }

    m_flag30 = false;
    m_havePath = false;
    m_request = new RequestState();
    m_request->m_action = 0;
    m_int38 = 0;
    m_position = {0.0f, 0.0f, 0.0f};
    m_positionValid = false;
    m_isIdle = false;
    m_flag4A = false;
    m_flag4B = false;
    m_formationDistanceSq = 900.0f;
    m_moved = false;
    m_time24 = 0.0;
    m_vector54 = {0.0f, 0.0f, 0.0f};
    m_vector60 = {0.0f, 0.0f, 0.0f};
}

// only these two stores. Retail does not free m_request or
// clear the bot's back-pointer here. Preserve both omissions rather than inventing ownership.
SquadMember::~SquadMember()
{
    m_player = 0;
    m_bot = 0;
}

IAIPlayer* SquadMember::getPlayer() { return m_player; }

// return the stored squad pointer.
// Non-const in the mangled signature; no lookup, ownership transfer or null guard.
SquadCoordinator* SquadMember::getSquad()
{
    return m_squad;
}

int SquadMember::getPlayerId() { return m_playerId; }
bool SquadMember::isBot() { return m_bot != 0; }
// stored pointer, no lookup or guard.
Bot* SquadMember::getBot()
{
    return m_bot;
}

int SquadMember::getMemberIndex() { return m_memberIndex; }
void SquadMember::setMemberIndex(int index) { m_memberIndex = index; }

// The distance is three-dimensional and compared squared against m_formationDistanceSq.
bool SquadMember::isCloseToNonOwnedTarget() const
{
    if (!m_squad->squadHasMission() || !m_squad->isSquadAssignedToAnyArea())
        return false;
    if (static_cast<int>(m_squad->getSquadStrategicAreaAssignment()->getStatus(m_side + 1)) == 0
        || !m_positionValid)
    {
        return false;
    }
    const Vec3& goal = m_squad->getSquadGotoPos();
    const float dx = m_position.x - goal.x;
    const float dy = m_position.y - goal.y;
    const float dz = m_position.z - goal.z;
    return dx * dx + dy * dy + dz * dz < m_formationDistanceSq;
}

void SquadMember::reset()
{
    m_positionValid = false;
    m_isIdle = true;
    m_flag4A = true;
    m_flag4B = false;
    m_time24 = 0.0;
    const Vec3 zero = { 0.0f, 0.0f, 0.0f };
    m_vector54 = zero;
    m_vector60 = zero;
}

void SquadMember::setIsIdle(bool idle)
{
    m_isIdle = idle;
}

bool SquadMember::getIsIdle()
{
    return m_isIdle;
}

// 0x1c56b0 (Ghidra).
int SquadMember::getSquadId()
{
    return m_squadIndex;
}

// 0x1c39e0 (Ghidra).
bool SquadMember::isSameRequest(unsigned int handle, int action)
{
    return handle == m_request->m_handle && action == m_request->m_action;
}

void SquadMember::clearRequestAction()
{
    m_request->m_action = 0;
    m_request->m_handle = 0xFFFFFFFFu;
}

// Frees the member's formation slot.
void SquadMember::squadMemberKilled()
{
    SquadFormation* const formation = m_squad->getSquadFormation();
    if (formation->isInFormation(m_memberIndex))
        formation->leaveFormation(m_memberIndex);
}

void SquadMember::setIsSquadLeader(bool leader) { m_isSquadLeader = leader; }
bool SquadMember::isSquadLeader() { return m_isSquadLeader; }

// the second flag receives the inverse, not a constant clear.
void SquadMember::setHavePath(bool havePath)
{
    m_havePath = havePath;
    m_flag4A = !havePath;
}

// [INFERRED] void return: coordinator discards the result and exits have no common
// return materialization. Each missing dependency stops at its real call site;
// paths which retail skips continue through mission selection and movement.
void SquadMember::update(double time)
{
    updateMemberPos();
    SquadFormation* const formation = m_squad->getSquadFormation();

    if (memberPathUpdateDue(m_time24, time))
    {
        Vec3 position = {};
        if (memberPathDisplaced(m_vector54, m_vector60))
        {
            setHavePath(false);
            m_vector54 = m_vector60;
        }
        else if (m_bot && getPosition(position))
        {
            if (m_bot->getWaypoints()->getUrgency(position, m_bot) < 0.5f)
                setHavePath(false);
            else if (!m_moved)
            {
                const float x = m_vector60.x - position.x;
                const float y = m_vector60.y - position.y;
                const float z = m_vector60.z - position.z;
                if (16.0f < x * x + y * y + z * z)
                    setHavePath(false);
            }
        }
        m_time24 = time;
    }

    if (m_bot && m_player && m_player->getIsAlive())
    {
        Vec3 position = {};
        if (getPosition(position))
        {
            double since = 0.0;
            int bySide = 0;
            if (isMobile()
                && SquadManager::m_instance->isInArtilleryDangerZone(position, m_side + 1, since, bySide))
            {
                if (avoidArtillery(position, since, bySide))
                    return;
            }
            else
            {
                m_flag4B = false;
            }
        }
    }

    if (m_bot && !m_bot->isFiring())
    {
        // [PLATFORM] PE reads the manager after getCurrentEquipment; ELF captures
        // it first. Both reload the bot after isFiring and preserve pool semantics.
#if defined(_WIN32)
        const unsigned int handle = m_bot->getCurrentEquipment();
        Information* const information = informationManager->getInformation(handle);
#else
        InformationManager* const manager = informationManager;
        const unsigned int handle = m_bot->getCurrentEquipment();
        Information* const information = manager->getInformation(handle);
#endif
        if (information && (information->getObjectTypeFlags() & 0x200000u) &&
            information->hasArmament())
        {
            switchToThirdWeapon(information);
        }
    }

    if (m_squad->getMissionStatus() == 1)
    {
        SquadMember* member = m_squad->getSquadLeader();
        if (!member || !member->getPlayer()->getIsAlive() || member->getPlayer()->isManDown())
        {
            member = this;
            if (!m_player->getIsAlive() || m_player->isManDown())
                member = nullptr;
        }
        if (member)
        {
            IBFSquad* const adapter = static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad);
            if (m_squad->acceptMission(member))
                adapter->acceptSquadOrder(m_squadIndex + 1, m_player);
            else
                adapter->declineSquadOrder(m_squadIndex + 1, m_player);
        }
    }

    // Formation remains the entry capture; member index is fresh after callbacks.
    const bool inFormation = formation->isInFormation(m_memberIndex);
    m_int38 = 0;
    if (inFormation)
        moveWithFormation();
    else
        moveWithoutFormation();
}

// No result
// is materialized. Capture the formation and allowance once, but query membership twice
// and reload the member index after every callback. A join/leave clears only havePath:
// setHavePath(false) would incorrectly change m_flag4A as well.
void SquadMember::updateFormationStatus()
{
    SquadFormation* const formation = m_squad->getSquadFormation();
    const bool allowed = isAllowedInFormation();
    if (formation->isInFormation(m_memberIndex) && !allowed)
    {
        formation->leaveFormation(m_memberIndex);
        m_havePath = false;
    }
    if (!formation->isInFormation(m_memberIndex) && allowed)
    {
        formation->joinFormation(m_memberIndex);
        m_havePath = false;
    }
}

// Copy all coordinates before
// returning the stored validity flag; false does not preserve the caller's old output.
bool SquadMember::getPosition(Vec3& position)
{
    position = m_position;
    return m_positionValid;
}

// one byte load, ELF +0x50 / PE +0x54.
bool SquadMember::isMoving()
{
    return m_moved;
}

// 0x1c5540 (Ghidra).
bool SquadMember::isAtFormationSlot()
{
    return m_flag30;
}

// 0x1c2e90 (Ghidra).
bool SquadMember::isAvoidingArtillery()
{
    return m_flag4B;
}

// 0x1c5380 (Ghidra). A living soldier, unless its squad's mission is to an area and it has come
// within the formation distance of the place there.
bool SquadMember::isAllowedInFormation()
{
    if (!m_player->getIsAlive() || m_player->isManDown())
        return false;
    IAIObject* const vehicle = m_player->getCurrentVehicle();
    if (!vehicle)
        return false;
    Information* const information = informationManager->getInformation(vehicle->getInformation(m_side + 1));
    if (!information || !information->getUnit() || (information->getObjectTypeFlags() & 0x200000) == 0
        || !information->getMobile())
        return false;
    Vec3 goal = m_squad->getSquadGotoPos();
    Vec3 position = {};
    if (m_squad->getMissionStatus() == 2 && m_squad->isSquadAssignedToAnyArea())
    {
        m_squad->getValidSaPos(this, goal);
        if (getPosition(position))
        {
            const float x = goal.x - position.x;
            const float y = goal.y - position.y;
            const float z = goal.z - position.z;
            if (z * z + x * x + y * y < m_formationDistanceSq)
                return false;
        }
    }
    return true;
}

// 0x1c1a70 (Ghidra). No null guard on the vehicle's record, as shipped. Moved: over 10 cm.
void SquadMember::updateMemberPos()
{
    IAIObject* const vehicle = m_player->getCurrentVehicle();
    m_positionValid = false;
    m_moved = false;
    if (!vehicle)
        return;
    Information* const record = informationManager->getInformation(vehicle->getInformation(m_side + 1));
    Information* const primary = informationManager->getInformation(record->getPrimaryObject(m_side + 1));
    if (!primary || !primary->getUnit())
        return;
    const Vec3 last = m_position;
    m_position = primary->getPosition();
    m_positionValid = true;
    const float x = last.x - m_position.x;
    const float y = last.y - m_position.y;
    const float z = last.z - m_position.z;
    if (0.01f < z * z + x * x + y * y)
        m_moved = true;
}

// 0x1c5560 (Ghidra).
bool SquadMember::isMobile()
{
    IAIObject* const vehicle = m_player->getCurrentVehicle();
    if (!vehicle || !m_player->getIsAlive() || m_player->isManDown())
        return false;
    Information* const information = informationManager->getInformation(vehicle->getInformation(m_side + 1));
    return information && information->getUnit() && information->getMobile();
}

// 0x1c3ac0 (Ghidra). Takes the speed and arrival estimate from the slot. No null guards on the
// vehicle, as shipped. The member in slot 0 leads to the goal; the others go to their slots when
// idle or told to look for a new path.
void SquadMember::moveWithFormation()
{
    SquadFormation* const formation = m_squad->getSquadFormation();
    Vec3 position = {};
    getPosition(position);
    Vec3 slot = {};
    if (!formation->getSlotWorldPos(m_memberIndex, slot))
        return;
    float x = slot.x - position.x;
    float y = slot.y - position.y;
    float z = slot.z - position.z;
    const float distanceSqr = y * y + x * x + z * z;
    Information* const equipment =
        informationManager->getInformation(m_player->getCurrentVehicle()->getInformation(m_side + 1));
    const float speed = equipment->getMobile()->getMaxSpeed();
    m_maxSpeed = speed;
    m_estimatedArrival = distanceSqr / speed;
    if (!m_bot)
        return;
    if (distanceSqr <= 10.0f)
    {
        if (!m_flag30)
            m_havePath = false;
        m_flag30 = true;
    }
    else
        m_flag30 = false;
    if (formation->getSquadMemberIndexAtSlot(0) == m_memberIndex)
    {
        if (m_squad->getMissionStatus() == 2 && formation->shouldSquadMoveOut())
        {
            m_maxSpeed = formation->getFormationMaxSpeed();
            Vec3 goal = m_squad->getSquadGotoPos();
            if (m_squad->isSquadAssignedToAnyArea())
                m_squad->getValidSaPos(this, goal);
            if (m_havePath)
                return;
            x = goal.x - position.x;
            y = goal.y - position.y;
            z = goal.z - position.z;
            if (z * z + x * x + y * y <= 8.0f)
                return;
        }
        else
        {
            if (m_havePath || !IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
                return;
        }
        moveToGoalPos(equipment);
    }
    else if (m_isIdle || m_flag4A)
    {
        m_flag4A = false;
        moveToSlotPos(slot, equipment);
    }
}

// 0x1c3e20 (Ghidra). No null guards on the vehicle, as shipped.
void SquadMember::moveWithoutFormation()
{
    if (!m_bot || !m_player->getIsAlive())
        return;
    Vec3 position = {};
    getPosition(position);
    const Vec3 goal = m_squad->getSquadGotoPos();
    Information* const equipment =
        informationManager->getInformation(m_player->getCurrentVehicle()->getInformation(m_side + 1));
    if (equipment->getMobile())
        m_maxSpeed = equipment->getMobile()->getMaxSpeed();
    const float x = goal.x - position.x;
    const float y = goal.y - position.y;
    const float z = goal.z - position.z;
    if (m_havePath)
        return;
    if (!(m_squad->getMissionStatus() == 2 && 8.0f < z * z + x * x + y * y)
        && !IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
        return;
    moveToGoalPos(equipment);
}

namespace {

// A route of one point on the ground, as every squad move starts.
WPMoveTo* newMove(const Vec3& point)
{
    std::list<std::pair<Vec3, int> > points;
    points.push_back(std::make_pair(point, -1));
    return checkPointer(new WPMoveTo(0.0f, 60.0f, 2, points));
}

// A flying vehicle's route: 70 m over the ground at a 30 m radius, a helicopter (flag 0x20) 20 m at 5 m.
WPAltitudeMoveTo* newFlight(const Vec3& point, float ground, Information* equipment)
{
    const bool helicopter = (equipment->getObjectTypeFlags() & 0x20) != 0;
    const float radius = helicopter ? 5.0f : 30.0f;
    const float height = helicopter ? 20.0f : 70.0f;
    std::list<Vec3> points;
    Vec3 high = point;
    high.y = ground + height;
    points.push_back(high);
    return checkPointer(new WPAltitudeMoveTo(radius, radius, height, 2, points));
}

// A bot's answer to its human leader, once per goal.
void answerLeader(SquadCoordinator* squad, SquadMember* member, int message)
{
    if (!squad->getRogerThatPositionMessage() || !member->isBot())
        return;
    member->getBot()->getCommunicationAgent()->enqueueRadioMessage(
        static_cast<AIRadioMessageType>(message), static_cast<AIRadioChannelType>(3), 3, 2.0f, 0);
    squad->setRogerThatPositionMessage(false);
}

} // namespace

// 0x1c3ff0 (Ghidra). As shipped, a flight takes the ground height at the slot's x and y.
void SquadMember::moveToSlotPos(const Vec3& slot, Information* equipment)
{
    m_havePath = true;
    m_vector60 = slot;
    if ((equipment->getObjectTypeFlags() & 0x10) == 0)
    {
        m_bot->setWaypoints(newMove(slot));
        return;
    }
    const IAITerrain* const terrain = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
    m_bot->setWaypoints(newFlight(slot, terrain->getTerrainAltitude(slot.x, slot.y), equipment));
}

// 0x1c4240 (Ghidra). On a mission, the route to the goal: a ground unit whose squad left its human
// leader goes to that leader's goal when it is valid and not inside the assigned area, or else to
// the squad's goal, the order position of the area when the squad has one. There a route of
// strategic objects is used when one is stored.
void SquadMember::moveToGoalPos(Information* equipment)
{
    if (!equipment->getMobile() || m_squad->getMissionStatus() != 2)
        return;
    AIStrategicArea* const assigned = m_squad->getSquadStrategicAreaAssignment();
    const bool flying = (equipment->getObjectTypeFlags() & 0x10) != 0;
    if (!flying && m_squad->getSquadFormation()->shouldLeaveSquadLeader() && !m_squad->isSquadCloseToNonOwnedTarget())
    {
        Vec3 goal = m_squad->getSquadLeaderGotoPos();
        bool tryGoal = true;
        if (assigned)
        {
            if (assigned->isInside(goal))
                tryGoal = false;
            else
            {
                AIStrategicArea* const closest = static_cast<AIStrategicArea*>(
                    AIStrategicObjectManager::getInstance()->getClosestObject(goal, m_side + 1));
                if (closest && closest->isInside(goal))
                    m_squad->getValidSaPos(this, closest, goal);
            }
        }
        if (tryGoal)
        {
            if (m_squad->isValidPosition(this, goal))
            {
                m_havePath = true;
                m_bot->setWaypoints(newMove(goal));
                m_vector60 = goal;
                answerLeader(m_squad, this, 6);
                return;
            }
            answerLeader(m_squad, this, 7);
        }
    }
    Vec3 goal = m_squad->getSquadGotoPos();
    if (m_squad->isSquadAssignedToAnyArea())
        m_squad->getValidSaPos(this, goal);
    if (flying)
    {
        const IAITerrain* const terrain = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
        m_bot->setWaypoints(newFlight(goal, terrain->getTerrainAltitude(goal.x, goal.z), equipment));
        m_vector60 = goal;
        answerLeader(m_squad, this, 6);
        return;
    }
    if (!m_squad->isValidPosition(this, goal))
    {
        answerLeader(m_squad, this, 7);
        return;
    }
    m_havePath = true;
    if (m_squad->isSquadAssignedToAnyArea())
    {
        std::list<AIStrategicObject*> route;
        const int botId = m_bot->getBotId();
        ISAI* const sai = IAIMain::m_instance->getSAI(m_side + 1);
        AIStrategicObject* const from =
            AIStrategicObjectManager::getInstance()->getStrategicObject(sai->getBotCurrentPos(botId));
        if (assigned->getRouteList(m_side + 1, from, route))
        {
            if (route.size() > 1)
            {
                Vec3 position = {};
                getPosition(position);
                const float dx = goal.x - position.x;
                const float dz = goal.z - position.z;
                const Vec2& first = route.front()->getPosition(m_side + 1);
                const float fx = goal.x - first.x;
                const float fz = goal.z - first.y;
                if (dx * dx + dz * dz < fz * fz + fx * fx)
                    route.pop_front();
            }
            goal.y = assigned->m_f148;
            m_vector60 = goal;
            assigned->orderNormalBot(m_side + 1, route, botId, sai);
            answerLeader(m_squad, this, 6);
            return;
        }
    }
    WPMoveTo* const move = newMove(goal);
    if (isSameWaypointList(m_bot->getWaypoints(), move))
    {
        delete move;
        return;
    }
    m_bot->setWaypoints(move);
    m_vector60 = goal;
    if (!m_squad->isSquadAssignedToAnyArea())
        answerLeader(m_squad, this, 6);
}

// 0x1c5120 (Ghidra).
bool SquadMember::isSameWaypointList(WaypointList* current, WaypointList* other)
{
    if (current->m_int0C != 0)
        return false;
    std::list<Vec3> a;
    std::list<Vec3> b;
    static_cast<WPMoveTo*>(current)->getCoords(a);
    static_cast<WPMoveTo*>(other)->getCoords(b);
    if (a.empty() || b.empty())
        return false;
    std::list<Vec3>::const_iterator p = a.begin();
    std::list<Vec3>::const_iterator q = b.begin();
    for (; p != a.end() && q != b.end(); ++p, ++q)
    {
        const float x = q->x - p->x;
        const float y = q->y - p->y;
        const float z = q->z - p->z;
        if (100.0f < z * z + x * x + y * y)
            return false;
    }
    return p == a.end() && q == b.end();
}

// 0x1c2840 (Ghidra). Away from where the shells fall to 5 m past the safe distance, turning the
// direction out by a growing random angle each side for five tries; then the last place the side
// fled to. Already fleeing: true.
bool SquadMember::avoidArtillery(const Vec3& position, double, int bySide)
{
    if (m_flag4B)
        return true;
    SquadManager* const manager = SquadManager::m_instance;
    const Vec3 target = manager->getArtilleryTargetPos(bySide);
    Vec3 away;
    away.x = position.x - target.x;
    away.y = position.y - target.y;
    away.z = position.z - target.z;
    const float reach = manager->getArtillerySafeDistance() + 5.0f;
    const float length2 = away.z * away.z + away.x * away.x + away.y * away.y;
    if (!(std::fabs(length2 - 1.0f) < 1.1920929e-07f))
    {
        if (1.1920929e-07f <= std::fabs(length2))
        {
            const float inverse = 1.0f / std::sqrt(length2);
            away.x = away.x * inverse;
            away.y = away.y * inverse;
            away.z = inverse * away.z;
        }
        else
        {
            away.x = 0.0f;
            away.y = 0.0f;
            away.z = 0.0f;
        }
    }
    Vec3 direction = away;
    Vec3 flee = {};
    float angle = 0.0f;
    bool found = false;
    for (int i = 0; i < 5 && !found; ++i)
    {
        flee.x = reach * direction.x + target.x;
        flee.y = reach * direction.y + target.y;
        flee.z = reach * direction.z + target.z;
        if (m_squad->isValidPosition(this, flee))
        {
            found = true;
            break;
        }
        const float step = AIRandom::m_instance->nextFloat() * 0.2f + 0.6f;
        if (angle <= 0.0f)
            angle = step + angle + angle + angle;
        else
            angle = (angle - (angle + angle)) - step;
        rotateXZ(away, direction, angle);
    }
    if (!found && !manager->haveValidArtilleryFleePos(this, flee))
        return false;
    m_havePath = true;
    m_flag4B = true;
    m_bot->setWaypoints(newMove(flee));
    if (found)
        manager->setValidArtilleryFleePos(this, flee);
    return true;
}

// 0x1c1ed0 (Ghidra), the update's weapon block. A soldier not firing presses its third weapon's
// key, unless a kit 5 member holds a weapon of type 1 with health to spare or a kit 3 member with
// type 2 in hand is short of ammunition. No null guard on the physical, as shipped.
void SquadMember::switchToThirdWeapon(Information* equipment)
{
    IPIArmament* const armament = equipment->getArmament();
    if (!armament || armament->getActiveWeapon() == -1)
        return;
    WeaponInfo* const active =
        WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));
    if (!active)
        return;
    IPIPhysical* const physical = equipment->getPhysical();
    bool stay = false;
    if (m_player->getSpawnKit() != 5 || active->getWeaponType() != 1 || 0.95f < physical->getHealth())
    {
        if (m_player->getSpawnKit() == 3 && active->getWeaponType() == 2)
        {
            const std::vector<unsigned int> weapons = armament->getWeapons();
            float left = 1.0f;
            for (unsigned int i = 0; i < weapons.size(); ++i)
            {
                WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
                if (weapon && weapon->getWeaponType() != 1 && weapon->getWeaponType() != 2 && weapon->getAmmo() != -1
                    && weapon->getMaxAmmo() != 0)
                    left = (static_cast<float>(weapon->getCurrentAmmo()) / static_cast<float>(weapon->getMaxAmmo())) * left;
            }
            stay = left <= 0.9f;
        }
        if (!stay && active->getWeaponType() != 1 && active->getWeaponType() != 2 && m_bot->isFiring())
            stay = true;
    }
    else
        stay = true;
    WeaponInfo* const third = WeaponInfoManager::m_instance->getInformation(armament->getWeapon(2));
    if (third && !stay && active != third)
        static_cast<BotMain*>(m_bot)->getPlayerInput()->setInput(
            static_cast<io::PlayerInputMap>(third->getWeaponActivate()), 1.0f);
}

// 0x1c3a10 (Ghidra). No null guards, as shipped.
float SquadMember::getVehicleRadius()
{
    Information* const information =
        informationManager->getInformation(m_player->getCurrentVehicle()->getInformation(m_side + 1));
    return information->getPhysical()->getRadius();
}

} // namespace ai
} // namespace hfe
} // namespace dice
