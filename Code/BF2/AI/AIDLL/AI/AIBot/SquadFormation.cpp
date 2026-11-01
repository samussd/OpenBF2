// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadFormation operations.

#include "SquadFormation.h"

#include <cmath>

#include <list>

#include "SquadCoordinator.h"
#include "SquadMember.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AIRandom.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"   // IAIPlayer::getCurrentVehicle
#include "../../../AIInterface/BFKnowledge/BFGame.h"     // IAIGame::getMaxNumberOfSquadMembers
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

float squaredDistance(const Vec3& a, const Vec3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

// attested type name: `std::list<dice::hfe::ai::(anonymous namespace)::SquadMemberDistanceObj>`
// is instantiated from this translation unit (the anonymous namespace carries its path). Eight
// bytes, from the list's 16-byte header node. Member names are [INFERRED].
struct SquadMemberDistanceObj
{
    // Both images' merge instantiations order by the float at +4, ascending, strict; an
    // unordered pair compares false and keeps the first list's element in front.
    bool operator<(const SquadMemberDistanceObj& other) const
    {
        return m_distanceSq < other.m_distanceSq;
    }

    SquadMember* m_member;
    float m_distanceSq;
};

} // anonymous namespace

// [INFERRED] local name. Both images retain every delta/product through the
// ordered comparison. PE adds (z*z+y*y)+x*x; ELF adds (y*y+x*x)+z*z.
// Native MSVC x86 is the validated target; portable arms are not ELF/x64 proof.
static bool formationLeaderWithinRange(const Vec3& leader, const Vec3& squad)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const leaderPosition = &leader;
    const Vec3* const squadPosition = &squad;
    const float threshold = 2500.0f;
    bool within = false;
    __asm {
        mov eax, leaderPosition
        mov edx, squadPosition
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st(0), st(0)
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld dword ptr [eax]
        fsub dword ptr [edx]
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
    const long double deltaX = static_cast<long double>(leader.x) - squad.x;
    const long double deltaY = static_cast<long double>(leader.y) - squad.y;
    const long double deltaZ = static_cast<long double>(leader.z) - squad.z;
#if defined(_WIN32)
    return (deltaZ * deltaZ + deltaY * deltaY) + deltaX * deltaX < 2500.0L;
#else
    return (deltaY * deltaY + deltaX * deltaX) + deltaZ * deltaZ < 2500.0L;
#endif
#endif
}

// [INFERRED] local name. Squared length ordered strictly above 0.001f; equality and NaN refuse.
// PE squares the stored components and sums (z*z+y*y)+x*x. ELF first subtracts a zero vector
// (exact) and sums (y*y+x*x)+z*z. Both keep the sum wide through the comparison.
// Native MSVC x86 is the validated target; portable arms are not ELF/x64 proof.
static bool trailPointIsSet(const Vec3& point)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const p = &point;
    const float threshold = 0.001f;
    bool set = false;
    __asm {
        mov eax, p
        fld dword ptr [eax+8]
        fmul st(0), st(0)
        fld dword ptr [eax+4]
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld dword ptr [eax]
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp threshold
        fnstsw ax
        test ah, 41h
        jnz notSet
        mov set, 1
    notSet:
    }
    return set;
#else
    const long double x = point.x;
    const long double y = point.y;
    const long double z = point.z;
#if defined(_WIN32)
    return (z * z + y * y) + x * x > static_cast<long double>(0.001f);
#else
    return (y * y + x * x) + z * z > static_cast<long double>(0.001f);
#endif
#endif
}

// [INFERRED] local name. Squared distance between the anchor and the newest crumb, narrowed
// to float before the caller's strict 36 test. PE subtracts through its float-returning Vec3
// operator (anchor - crumb) and sums (dx*dx+dy*dy)+dz*dz; ELF keeps crumb - anchor wide and
// sums (dy*dy+dx*dx)+dz*dz. The sign of each difference cannot matter once squared.
static float trailCrumbDistanceSq(const Vec3& anchor, const Vec3& crumb)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const a = &anchor;
    const Vec3* const c = &crumb;
    float deltaX, deltaY, deltaZ, distance;
    __asm {
        mov eax, a
        mov edx, c
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
    const long double dx = static_cast<float>(static_cast<long double>(anchor.x) - crumb.x);
    const long double dy = static_cast<float>(static_cast<long double>(anchor.y) - crumb.y);
    const long double dz = static_cast<float>(static_cast<long double>(anchor.z) - crumb.z);
    return static_cast<float>((dx * dx + dy * dy) + dz * dz);
#else
    const long double dx = static_cast<long double>(crumb.x) - anchor.x;
    const long double dy = static_cast<long double>(crumb.y) - anchor.y;
    const long double dz = static_cast<long double>(crumb.z) - anchor.z;
    return static_cast<float>((dy * dy + dx * dx) + dz * dz);
#endif
#endif
}

// [INFERRED] local name. A claimed member's squared distance to the anchor, stored as the sort
// key. PE spills only dx to float and sums (dz*dz+dy*dy)+dx*dx; ELF keeps all three
// differences wide and sums (dy*dy+dx*dx)+dz*dz. Both narrow the sum to float for the key.
static float trailMemberDistanceSq(const Vec3& position, const Vec3& anchor)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const p = &position;
    const Vec3* const a = &anchor;
    float deltaX, distance;
    __asm {
        mov eax, p
        mov edx, a
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
        fstp distance
    }
    return distance;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(position.x) - anchor.x);
#else
    const long double dx = static_cast<long double>(position.x) - anchor.x;
#endif
    const long double dy = static_cast<long double>(position.y) - anchor.y;
    const long double dz = static_cast<long double>(position.z) - anchor.z;
#if defined(_WIN32)
    return static_cast<float>((dz * dz + dy * dy) + dx * dx);
#else
    return static_cast<float>((dy * dy + dx * dx) + dz * dz);
#endif
#endif
}

// [INFERRED] Runs the formation helpers, then advances a signed six-tick cycle: tick 3
// selects a formation and tick 6 resets the counter and applies it. [PLATFORM] The PE
// inlines selection; ELF calls it. Both use the same integer policy.
void SquadFormation::update(double time)
{
    updateSlotZeroPos();
    updateMovementFlag(false);
    updateFormationMaxSpeed();
    updateFormation();
    updateWaitForSquadMembers(time);
    updateFormationForwardVector();
    updateTrailingPositions();
    updateTrailingSlots();

    if (++m_i88 > 5)
    {
        m_i88 = 0;
        setFormation();
    }
    else if (m_i88 == 3)
        selectFormation();
}

// Selects the line formation unconditionally. The next cycle wrap applies it, replacing
// any shape selected by changeFormation.
void SquadFormation::selectFormation()
{
    m_formation = FormationLine;
}

// Points the formation along its anchor member's motion. An empty anchor or one that is almost
// still leaves the direction as it is.
void SquadFormation::updateFormationForwardVector()
{
    Slot* const anchor = m_slots[0];
    if (!anchor->m_occupied)
        return;

    IAIPlayer* const player = m_coordinator->getSquadMember(anchor->m_memberIndex)->getPlayer();
    IAIObject* const vehicle = player->getCurrentVehicle();
    Information* const information =
        informationManager->getInformation(vehicle->getInformation(m_side + 1));
    const Vec3 speed = information->getMobile()->getSpeed();
    if (speed.x * speed.x + speed.y * speed.y + speed.z * speed.z <= 0.25f)
        return;

    m_f48 = speed.x;
    m_f52 = speed.z;
    const float lengthSq = m_f48 * m_f48 + m_f52 * m_f52;
    if (std::fabs(lengthSq - 1.0f) < 1.1920929e-07f)
        return;
    if (std::fabs(lengthSq) < 1.4210855e-14f)
    {
        m_f48 = 0.0f;
        m_f52 = 0.0f;
    }
    else
    {
        const float scale = 1.0f / std::sqrt(lengthSq);
        m_f52 *= scale;
        m_f48 *= scale;
    }
}

// Adds a trail position when the anchor moves more than six metres from the last position.
void SquadFormation::updateTrailingPositions()
{
    Slot* const anchor = m_slots[0];
    if (!trailPointIsSet(m_v72))
        return;
    if (!trailPointIsSet(m_trailingPosition))
    {
        m_trailingPosition = m_v72;
        return;
    }
    if (!anchor->m_occupied)
        return;

    const float distanceSq = trailCrumbDistanceSq(m_v72, m_trailingPosition);
    SquadMember* const member = m_coordinator->getSquadMember(anchor->m_memberIndex);
    (void)member;
    if (!(distanceSq > 36.0f))
        return;

    for (unsigned int index = m_maxSquadMembers - 1; index != 0; --index)
    {
        // Two statements: both images reload the vector's storage after the byte store.
        m_slotTargets[index].first = m_slotTargets[index - 1].first;
        m_slotTargets[index].second = m_slotTargets[index - 1].second;
    }
    m_slotTargets[0].first = true;
    m_slotTargets[0].second = m_trailingPosition;
    m_trailingPosition = m_v72;
}

// Reassigns occupied trailing slots according to distance.
void SquadFormation::updateTrailingSlots()
{
    std::list<SquadMemberDistanceObj> members;
    Vec3 position = {};

    for (unsigned int index = 0; index < m_maxSquadMembers; ++index)
    {
        Slot* const slot = m_slots[index];
        if (slot->m_i20 != -1 && slot->m_occupied)
        {
            SquadMember* const member = m_coordinator->getSquadMember(slot->m_memberIndex);
            member->getPosition(position);
            const SquadMemberDistanceObj entry = { member, trailMemberDistanceSq(position, m_v72) };
            members.push_back(entry);
        }
    }

    if (members.empty())
        return;

    members.sort();

    Slot* slot = 0;
    for (unsigned int index = 0; index < m_maxSquadMembers; ++index)
    {
        slot = m_slots[index];
        slot->m_i20 = -1;
    }

    int trailIndex = 0;
    for (std::list<SquadMemberDistanceObj>::iterator it = members.begin(); it != members.end(); ++it)
    {
        SquadMember* const member = it->m_member;
        for (unsigned int index = 1; index < m_maxSquadMembers; ++index)
        {
            slot = m_slots[index];
            if (slot->m_memberIndex == member->getMemberIndex())
                break;
        }
        slot->m_i20 = trailIndex++;
    }
}

// No bounds, null or occupied test: an empty
// slot answers its stored index, -1 after construction or reset.
int SquadFormation::getSquadMemberIndexAtSlot(int slotIndex)
{
    return m_slots[slotIndex]->m_memberIndex;
}

// 0x1bb6f0 (Ghidra).
bool SquadFormation::shouldSquadMoveOut()
{
    return m_b36;
}

bool SquadFormation::shouldLeaveSquadLeader()
{
    return m_b84;
}

// (, updateMovementFlag(true) inlined).
// The joined flag is set before the leave flag is cleared, in both images.
void SquadFormation::joinSquadLeader()
{
    m_b85 = true;
    m_b84 = false;
    updateMovementFlag(true);
}

// [INFERRED] void result. No slot-vector, leader or member null guards exist.
void SquadFormation::updateFormation()
{
    Slot* slot = m_slots[0];
    SquadMember* const leader = m_coordinator->getSquadLeader();
    const int leaderIndex = leader->getMemberIndex();
    const bool leaderIsBot = leader->isBot();
    bool leaderPresent = false;
    Vec3 squadPosition = {};
    m_coordinator->getSquadPosition(squadPosition);

    unsigned int leaderSlotIndex = 0;
    for (; leaderSlotIndex < m_maxSquadMembers; ++leaderSlotIndex)
    {
        slot = m_slots[leaderSlotIndex];
        if (slot->m_occupied && slot->m_memberIndex == leaderIndex)
        {
            leaderPresent = true;
            break;
        }
    }

    if (leaderPresent)
    {
        if (m_b84)
        {
            if (leaderSlotIndex != 0)
                return;
            for (unsigned int index = m_maxSquadMembers - 1; index > 0; --index)
            {
                Slot* const other = m_slots[index];
                if (!other->m_occupied)
                    continue;
                const int otherIndex = other->m_memberIndex;
                const int oldIndex = slot->m_memberIndex;
                slot->m_b24 = false;
                slot->m_i28 = 0.0f;
                slot->m_memberIndex = otherIndex;
                other->m_b24 = false;
                other->m_i28 = 0.0f;
                other->m_memberIndex = oldIndex;

                // This arm resets trailing positions before either path clear.
                resetTrailingPositions();
                m_coordinator->getSquadMember(slot->m_memberIndex)->setHavePath(false);
                m_coordinator->getSquadMember(other->m_memberIndex)->setHavePath(false);
                return;
            }
            return;
        }

        if (leaderSlotIndex == 0)
            return;
        Vec3 leaderPosition = {};
        leader->getPosition(leaderPosition); // Retail deliberately ignores validity.
        if (!formationLeaderWithinRange(leaderPosition, squadPosition) && !m_b85)
            return;

        m_b85 = false;
        Slot* const zero = m_slots[0];
        const bool oldOccupied = zero->m_occupied;
        const int oldIndex = zero->m_memberIndex;
        zero->m_occupied = true;
        zero->m_memberIndex = leaderIndex;
        zero->m_b24 = false;
        zero->m_i28 = 0.0f;
        m_coordinator->getSquadMember(leaderIndex)->setHavePath(false);

        // The old leader-slot pointer is reloaded after that callback.
        slot = m_slots[leaderSlotIndex];
        slot->m_occupied = oldOccupied;
        slot->m_memberIndex = oldIndex;
        slot->m_b24 = false;
        slot->m_i28 = 0.0f;
        // Even an unoccupied former anchor gets this unchecked member lookup.
        m_coordinator->getSquadMember(oldIndex)->setHavePath(false);
        resetTrailingPositions();
        return;
    }

    Slot* const zero = m_slots[0];
    if (!zero->m_occupied)
        return;
    if (m_coordinator->getSquadMember(zero->m_memberIndex)->isBot() == leaderIsBot)
        return;

    // Find the first occupied slot whose bot/human kind matches the captured leader.
    // The bound, vector and coordinator are fresh after each member callback.
    for (unsigned int index = 1; index < m_maxSquadMembers; ++index)
    {
        Slot* const other = m_slots[index];
        if (!other->m_occupied ||
            m_coordinator->getSquadMember(other->m_memberIndex)->isBot() != leaderIsBot)
            continue;
        const int oldIndex = zero->m_memberIndex;
        const int otherIndex = other->m_memberIndex;
        other->m_memberIndex = oldIndex;
        other->m_i28 = 0.0f;
        other->m_b24 = false;
        zero->m_memberIndex = otherIndex;
        zero->m_b24 = false;
        zero->m_i28 = 0.0f;
        m_coordinator->getSquadMember(otherIndex)->setHavePath(false);
        resetTrailingPositions();
        return;
    }
}

// [INFERRED] void result. The first slot is deliberately excluded from this scan.
void SquadFormation::updateWaitForSquadMembers(double time)
{
    m_b36 = true;
    Vec3 memberPosition = {};
    Vec3 squadPosition = {};
    for (unsigned int index = 1; index < m_maxSquadMembers; ++index)
    {
        Slot* const slot = m_slots[index];
        if (!slot->m_occupied)
            continue;

        SquadMember* const member = m_coordinator->getSquadMember(slot->m_memberIndex);
        bool late = !(m_b84 && slot->m_memberIndex == m_coordinator->getSquadLeaderIndex())
                 && !member->isBot();
        if (late)
        {
            const float arrival = member->getEstimatedArrival();
            const bool positioned = member->getPosition(memberPosition);
            m_coordinator->getSquadPosition(squadPosition);
            late = positioned && arrival < 1500.0f && arrival > 150.0f
                && squaredDistance(memberPosition, m_coordinator->getSquadGotoPos())
                       > squaredDistance(squadPosition, m_coordinator->getSquadGotoPos());
        }
        if (!late)
        {
            slot->m_b24 = false;
            slot->m_i28 = 0.0f;
        }
        else if (slot->m_b24)
        {
            if (static_cast<float>(time) > slot->m_i28)
                slot->m_b24 = false;
            else
                m_b36 = false;
        }
        else if (slot->m_i28 < 0.01f)
        {
            m_b36 = false;
            slot->m_b24 = true;
            slot->m_i28 = AIRandom::m_instance->nextFloat() * 10.0f + 10.0f
                        + static_cast<float>(time);
        }
    }
}

// [INFERRED] local helper name. Preserve leaveFormation's x87 comparison before its
// conditional float store. [PLATFORM] PE spills only dx to float; ELF keeps all three differences
// in x87 registers. The MSVC x86 block avoids modern SSE narrowing of dy/dz and the sum.
static bool nearerFormationMember(const Vec3& position, const Vec3& destination, float& minimum)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const positionPointer = &position;
    const Vec3* const destinationPointer = &destination;
    float* const minimumPointer = &minimum;
    float deltaX;
    bool nearer = false;
    __asm {
        mov eax, positionPointer
        mov edx, destinationPointer
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fmul st(0), st(0)
        fld deltaX
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st(0), st(0)
        faddp st(1), st(0)
        mov ecx, minimumPointer
        fcom dword ptr [ecx]
        fnstsw ax
        test ah, 5
        jp notNearer
        fstp dword ptr [ecx]
        mov nearer, 1
        jmp compared
    notNearer:
        fstp st(0)
    compared:
    }
    return nearer;
#else
    // Native long double retains the ELF x87 intermediate width. The separate MSVC
    // x64 port has no x87 inline assembly; it is not validated as retail x86 here.
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(position.x) - destination.x);
#else
    const long double dx = static_cast<long double>(position.x) - destination.x;
#endif
    const long double dy = static_cast<long double>(position.y) - destination.y;
    const long double dz = static_cast<long double>(position.z) - destination.z;
    const long double distance = (dy * dy + dx * dx) + dz * dz;
    if (distance < minimum)
    {
        minimum = static_cast<float>(distance);
        return true;
    }
    return false;
#endif
}

// Captured unsigned slot count, full signed member
// comparison, no occupied/null/vector-size test. Even an empty slot's -1 can match.
bool SquadFormation::isInFormation(int memberIndex)
{
    const unsigned int count = m_maxSquadMembers;
    for (unsigned int i = 0; i < count; ++i)
        if (m_slots[i]->m_memberIndex == memberIndex)
            return true;
    return false;
}

// Preserve offset
// reference-slot and target-vector state. [INFERRED] void return from discarded uses.
void SquadFormation::resetSlot(int slotIndex)
{
    Slot* const slot = m_slots[slotIndex];
    slot->m_occupied = false;
    slot->m_memberIndex = -1;
    slot->m_i20 = -1;
    slot->m_b24 = false;
    slot->m_i28 = 0.0f;
}

// Clears every slot, then gives the slots in order to the members allowed in formation. At most
// one fewer member than there are slots is visited, as shipped; the PE inlines resetSlot.
void SquadFormation::reassignSlots()
{
    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
        resetSlot(static_cast<int>(i));

    unsigned int count = static_cast<unsigned int>(m_coordinator->getNumberOfSquadMembers());
    if (m_maxSquadMembers <= count)
        count = m_maxSquadMembers - 1;

    int next = 0;
    for (unsigned int i = 0; i < count; ++i)
    {
        SquadMember* const member = m_coordinator->getSquadMember(static_cast<int>(i));
        if (member->isAllowedInFormation())
        {
            Slot* const slot = m_slots[next];
            slot->m_occupied = true;
            slot->m_memberIndex = member->getMemberIndex();
            ++next;
        }
    }
}

// [INFERRED]
// void return from discarded calls and inconsistent exit registers. Zero slots, null slots
// and bad member indices retain the retail missing guards. A missing member still updates
// movement; duplicate member indices stop at their first slot.
void SquadFormation::leaveFormation(int memberIndex)
{
    unsigned int removed = 0;
    for (; removed < m_maxSquadMembers; ++removed)
    {
        if (m_slots[removed]->m_memberIndex == memberIndex)
        {
            resetSlot(static_cast<int>(removed));
            break;
        }
    }

    if (removed < m_maxSquadMembers - 1 && m_slots[removed + 1]->m_occupied)
    {
        unsigned int last = m_maxSquadMembers - 1;
        while (last > 0)
        {
            Slot* const lastSlot = m_slots[last];
            if (lastSlot->m_occupied)
            {
                if (removed == 0 && last > 1)
                {
                    unsigned int selected = last;
                    float minimum = 3.402823466e+38F;
                    Vec3 position = {};
                    for (unsigned int candidate = last; candidate > 0; --candidate)
                    {
                        Slot* const slot = m_slots[candidate];
                        // Retail obtains the member even for an unoccupied candidate.
                        SquadMember* const member = m_coordinator->getSquadMember(slot->m_memberIndex);
                        if (slot->m_occupied && !member->isSquadLeader() && member->isBot() &&
                            member->getPosition(position))
                        {
                            const Vec3& destination = m_coordinator->getSquadGotoPos();
                            if (nearerFormationMember(position, destination, minimum))
                                selected = candidate;
                        }
                    }

                    Slot* destination = m_slots[0];
                    Slot* source = m_slots[selected];
                    destination->m_occupied = source->m_occupied;
                    destination->m_b24 = source->m_b24;
                    destination->m_offset = source->m_offset;
                    destination->m_memberIndex = source->m_memberIndex;
                    destination->m_i20 = source->m_i20;
                    destination->m_i28 = source->m_i28;
                    m_coordinator->getSquadMember(destination->m_memberIndex)->setHavePath(false);
                    resetSlot(static_cast<int>(selected));

                    if (selected < m_maxSquadMembers - 1 && m_slots[selected + 1]->m_occupied)
                    {
                        unsigned int replacement = m_maxSquadMembers - 1;
                        while (replacement > 0)
                        {
                            source = m_slots[replacement];
                            if (source->m_occupied)
                            {
                                destination = m_slots[selected];
                                destination->m_occupied = source->m_occupied;
                                destination->m_b24 = source->m_b24;
                                destination->m_offset = source->m_offset;
                                destination->m_memberIndex = source->m_memberIndex;
                                destination->m_i20 = source->m_i20;
                                destination->m_i28 = source->m_i28;
                                m_coordinator->getSquadMember(destination->m_memberIndex)->setHavePath(false);
                                resetSlot(static_cast<int>(replacement));
                                break;
                            }
                            --replacement;
                        }
                    }
                }
                else
                {
                    const int movedMember = lastSlot->m_memberIndex;
                    const int oldIndex = lastSlot->m_i20;
                    const bool oldFlag = lastSlot->m_b24;
                    const float oldValue = lastSlot->m_i28;
                    resetSlot(static_cast<int>(last));
                    Slot* const slot = m_slots[removed];
                    slot->m_occupied = true;
                    slot->m_memberIndex = movedMember;
                    slot->m_i20 = oldIndex;
                    slot->m_b24 = oldFlag;
                    slot->m_i28 = oldValue;
                    m_coordinator->getSquadMember(movedMember)->setHavePath(false);
                }
                break;
            }
            --last;
        }
    }
    if (removed == 0)
        resetTrailingPositions();
    updateMovementFlag(true);
}

// Slot zero is never considered. Fresh bounds after
// callbacks; occupied candidates only, with -1 and identical-slot results skipped.
void SquadFormation::updateClosestSlots()
{
    for (unsigned int slot = 1; slot < m_maxSquadMembers; ++slot)
    {
        if (m_slots[slot]->m_occupied)
        {
            const int closest = getClosestSlotIndex(static_cast<int>(slot));
            if (closest != -1 && closest != static_cast<int>(slot))
                swapSlots(static_cast<int>(slot), closest);
        }
    }
}

// The occupied slot, past the anchor, whose member stands nearest this slot's world position;
// -1 when there is none. No argument or null guard is added.
int SquadFormation::getClosestSlotIndex(int slotIndex)
{
    int best = -1;
    float minimum = 3.402823466e+38F;
    Vec3 target = {};
    Vec3 position = {};
    if (!getSlotWorldPos(m_slots[slotIndex]->m_memberIndex, target))
        return best;

    for (unsigned int index = 1; index < m_maxSquadMembers; ++index)
    {
        Slot* const slot = m_slots[index];
        if (slot->m_occupied
            && m_coordinator->getSquadMember(slot->m_memberIndex)->getPosition(position)
            && nearerFormationMember(position, target, minimum))
            best = static_cast<int>(index);
    }
    return best;
}

// The coordinator goes first; the slots were each allocated by the constructor.
SquadFormation::~SquadFormation()
{
    m_coordinator = 0;
    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
        delete m_slots[i];
}

// Keep the real allocation and captured pointers;
// only these three payload fields move, even for identical or aliased slots.
void SquadFormation::swapSlots(int first, int second)
{
    Slot* const a = m_slots[first];
    Slot* const b = m_slots[second];
    Slot* const temporary = new Slot(Slot::SwapTemporary());
    temporary->m_memberIndex = a->m_memberIndex;
    temporary->m_b24 = a->m_b24;
    temporary->m_i28 = a->m_i28;
    a->m_memberIndex = b->m_memberIndex;
    a->m_b24 = b->m_b24;
    a->m_i28 = b->m_i28;
    b->m_memberIndex = temporary->m_memberIndex;
    b->m_b24 = temporary->m_b24;
    b->m_i28 = temporary->m_i28;
    delete temporary;
}

// Allocates formation slots and parallel position state for the configured squad size.

SquadFormation::SquadFormation(SquadCoordinator* coordinator)
    : m_slots()
    , m_coordinator(coordinator)
    , m_formation(0)
    , m_f48(0.0f)
    , m_f52(0.0f)
    , m_i88(0)
    , m_d92(0.0)
    , m_slotTargets()
    , m_trailingPosition()
{
    m_direction.x = 0.0f;
    m_direction.y = 0.0f;
    m_direction.z = 0.0f;

    m_v60.x = 0.0f;
    m_v60.y = 0.0f;
    m_v60.z = 0.0f;
    m_v72.x = 0.0f;
    m_v72.y = 0.0f;
    m_v72.z = 0.0f;

    m_side = coordinator->getSide();

    m_maxSquadMembers = static_cast<unsigned int>(
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getMaxNumberOfSquadMembers());

    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        m_slots.push_back(new Slot());

        Vec3 zero;
        zero.x = 0.0f;
        zero.y = 0.0f;
        zero.z = 0.0f;
        m_slotTargets.push_back(std::pair<bool, Vec3>(false, zero));
    }

    // FLT_MAX, written as the bit pattern the binary stores.
    m_formationMaxSpeed = 3.402823466e+38F;
    updateFormationMaxSpeed();

    m_formation = FormationLine;
    setFormation();

    m_b36 = false;
    m_f48 = 0;
    m_f52 = 1.0f;
    m_v60.x = 0.0f;
    m_v60.y = 0.0f;
    m_v60.z = 0.0f;
    m_b56 = false;
    m_b84 = false;
    m_b85 = false;
    m_i88 = 0;
    m_d92 = 0.0;
    m_v72.x = 0.0f;
    m_v72.y = 0.0f;
    m_v72.z = 0.0f;
}

float SquadFormation::getFormationMaxSpeed()
{
    return m_formationMaxSpeed;
}

// Use the first free slot; neither a duplicate-member
// check nor a full-formation refusal exists. The movement update and path reset run
// even when the scan finds no free slot. PE inlines updateMovementFlag(true).
void SquadFormation::joinFormation(int memberIndex)
{
    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        Slot* const slot = m_slots[i];
        if (!slot->m_occupied)
        {
            slot->m_occupied = true;
            slot->m_memberIndex = memberIndex;
            slot->m_i20 = -1;
            slot->m_b24 = false;
            slot->m_i28 = 0;
            break;
        }
    }

    updateMovementFlag(true);
    m_coordinator->getSquadMember(memberIndex)->setHavePath(false);
}

// An unoccupied slot zero leaves the old movement
// flag untouched. Otherwise clear it before reading the clock, then set it only
// when forced or when both strict thresholds pass. No empty-slot-vector guard.
void SquadFormation::updateMovementFlag(bool force)
{
    if (!m_slots[0]->m_occupied)
        return;

    m_b56 = false;
    const double now =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    bool update = force;
    if (!update)
    {
        const float dx = m_v60.x - m_v72.x;
        const float dy = m_v60.y - m_v72.y;
        const float dz = m_v60.z - m_v72.z;
        // The PE sums z squared, y squared, then x squared.
        update = dz * dz + dy * dy + dx * dx > 4.0f && now > m_d92;
    }

    if (update)
    {
        m_b56 = true;
        m_d92 = now + 0.2;
        m_v60 = m_v72;
    }
}

// Preserve both guards.
// Keeps slot zero's world position current while the squad has one.
void SquadFormation::updateSlotZeroPos()
{
    if (m_slots[0]->m_occupied && m_coordinator->squadHasPosition())
        getSlotWorldPos(getSquadMemberIndexAtSlot(0), m_v72);
}

// Only the target-valid flags and slot indices are
// cleared; target coordinates and occupied/member-index fields remain unchanged.
void SquadFormation::resetTrailingPositions()
{
    updateSlotZeroPos();
    m_trailingPosition = m_v72;

    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        m_slotTargets[i].first = false;
        m_slots[i]->m_i20 = -1;
    }
}

// Sets the formation speed from the slowest occupied slot, adjusted for crowding.
void SquadFormation::updateFormationMaxSpeed()
{
    int occupied = 0;

    m_formationMaxSpeed = 3.402823466e+38F;

    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        Slot* const slot = m_slots[i];

        if (slot->m_occupied)
        {
            ++occupied;
            IAIObject* const vehicle =
                m_coordinator->getSquadMember(slot->m_memberIndex)->getPlayer()->getCurrentVehicle();
            Information* const information =
                informationManager->getInformation(vehicle->getInformation(m_side + 1));
            const float speed = information->getMobile()->getMaxSpeed();
            if (speed < m_formationMaxSpeed)
                m_formationMaxSpeed = speed;
        }
    }

    if (occupied)
    {
        if (occupied > 1)
            m_formationMaxSpeed = 0.89999998f * m_formationMaxSpeed;
    }
    else
    {
        m_formationMaxSpeed = 5.0f;
    }
}

// Sets the formation-specific direction and dispatches layout. Echelons mirror the
// third direction component; unknown formation values leave the layout unchanged.
void SquadFormation::setFormation()
{
    if (m_formation == FormationWedge)
    {
        m_direction.x = 0.80000001f;
        m_direction.y = 0.0f;
        m_direction.z = -0.60000002f;
        setWedgeFormation();
        return;
    }

    if (m_formation > FormationWedge)
    {
        if (m_formation == FormationEchelonA)
        {
            m_direction.x = 0.60000002f;
            m_direction.y = 0.0f;
            m_direction.z = -0.80000001f;
        }
        else
        {
            if (m_formation != FormationEchelonB)
                return;

            m_direction.x = 0.60000002f;
            m_direction.y = 0.0f;
            m_direction.z = 0.80000001f;
        }

        setEchelonFormation();
        return;
    }

    if (m_formation == FormationLine)
    {
        m_direction.x = 1.0f;
        m_direction.y = 0.0f;
        m_direction.z = 0.0f;
        setLineFormation();
    }
}

// Places slot zero at the origin and alternates the rest on either side of their reference
// slots, spread by both vehicles' radii. rightZ and leftZ scale the direction's z on each side.
void SquadFormation::layOutSlots(float rightZ, float leftZ)
{
    bool  positiveSide  = true;
    float offsetX       = 0.0f;
    float offsetZ       = 0.0f;
    int   referenceSlot = 0;

    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        Slot* const slot = m_slots[i];

        if (i)
        {
            slot->m_referenceSlot = referenceSlot;
            if (positiveSide)
            {
                offsetX = m_direction.x;
                offsetZ = rightZ * m_direction.z;
                positiveSide = false;
                if (i >= 3)
                    ++referenceSlot;
            }
            else
            {
                offsetX = -m_direction.x;
                offsetZ = leftZ * m_direction.z;
                positiveSide = true;
                ++referenceSlot;
            }
        }

        slot->m_offset.x = offsetX;
        slot->m_offset.y = 0.0f;
        slot->m_offset.z = offsetZ;

        if (slot->m_occupied)
        {
            const float own =
                3.0f * m_coordinator->getSquadMember(slot->m_memberIndex)->getVehicleRadius();
            Slot* const reference = m_slots[slot->m_referenceSlot];
            const float spread = own
                + 3.0f * m_coordinator->getSquadMember(reference->m_memberIndex)->getVehicleRadius();
            slot->m_offset.x = spread * slot->m_offset.x + reference->m_offset.x;
            slot->m_offset.y = spread * slot->m_offset.y + reference->m_offset.y;
            slot->m_offset.z = spread * slot->m_offset.z + reference->m_offset.z;
        }
    }
}

void SquadFormation::setLineFormation()
{
    layOutSlots(0.0f, 0.0f);
}

void SquadFormation::setWedgeFormation()
{
    layOutSlots(1.0f, 1.0f);
}

void SquadFormation::setEchelonFormation()
{
    layOutSlots(-1.0f, 1.0f);
}

// 0x1bb640 (Ghidra).
void SquadFormation::leaveSquadLeader()
{
    m_b85 = false;
    m_b84 = true;
    updateMovementFlag(true);
}

// 0x1bc550 (Ghidra). A squad at the origin has no position yet: the leader's. Slot 0 takes its spot
// as it is; another that falls off the mesh or in water takes a trailing place, and without one the
// mission's target or the first slot's member. The offset turns with the forward vector.
bool SquadFormation::getSlotWorldPos(int memberIndex, Vec3& position)
{
    Vec3 anchor;
    anchor.x = 0.0f;
    anchor.y = 0.0f;
    anchor.z = 0.0f;
    m_coordinator->getSquadPosition(anchor);
    if (anchor.x * anchor.x + anchor.y * anchor.y + anchor.z * anchor.z < 0.001f)
        return m_coordinator->getSquadLeader()->getPosition(position);
    float x = m_f48;
    float y = m_f52;
    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        Slot* const slot = m_slots[i];
        if (slot->m_memberIndex != memberIndex)
            continue;
        const Vec3 offset = slot->m_offset;
        const float length2 = x * x + y * y;
        if (!(std::fabs(length2 - 1.0f) < 1.1920929e-07f))
        {
            if (1.4210855e-14f <= std::fabs(length2))
            {
                const float inverse = 1.0f / std::sqrt(length2);
                x = x * inverse;
                y = inverse * y;
            }
            else
            {
                x = 0.0f;
                y = 0.0f;
            }
        }
        float cosine = y;
        if (cosine < -1.0f)
            cosine = -1.0f;
        else if (1.0f < cosine)
            cosine = 1.0f;
        float angle = std::acos(cosine);
        if (0.0f < x)
            angle = -angle;
        const float sine = std::sin(angle);
        position.x = anchor.x + (offset.x * y - offset.z * sine);
        position.y = offset.y + anchor.y;
        position.z = anchor.z + offset.x * sine + offset.z * y;
        if (i == 0)
            return true;
        SquadMember* const member = m_coordinator->getSquadMember(slot->m_memberIndex);
        if (m_coordinator->isValidPosition(member, position) && !m_coordinator->isPosOnWater(member, position))
        {
            slot->m_i20 = -1;
            return true;
        }
        int trail = slot->m_i20;
        if (trail == -1)
        {
            trail = getFirstFreeTrailingSlot(member);
            if (trail == -1)
            {
                if (m_coordinator->squadHasMission())
                    return m_coordinator->getSquadMissionTargetPos(position);
                SquadMember* const first = m_coordinator->getSquadMember(getSquadMemberIndexAtSlot(0));
                return first && first->getPosition(position);
            }
            slot->m_i20 = trail;
        }
        position = m_slotTargets[trail].second;
        return true;
    }
    return true;
}

// 0x1bca30 (Ghidra). As shipped, the position is tested first.
int SquadFormation::getFirstFreeTrailingSlot(SquadMember* member)
{
    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        const bool assigned = isTrailPosAssigned(static_cast<int>(i));
        const bool valid = m_coordinator->isValidPosition(member, m_slotTargets[i].second);
        if (m_slotTargets[i].first && !assigned && valid)
            return static_cast<int>(i);
    }
    return -1;
}

// 0x1bcb70 (Ghidra).
bool SquadFormation::isTrailPosAssigned(int index)
{
    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
        if (m_slots[i]->m_i20 == index)
            return true;
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
