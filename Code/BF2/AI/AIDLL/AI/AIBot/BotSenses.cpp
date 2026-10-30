// A bot's perception: hearing, sight and the shots it noticed. forgetObject (473 B) is not
// recovered: nothing calls it yet.

#include "BotSenses.h"

#include <cmath>

#include "BotMain.h"
#include "SensedObject.h"
#include "SensingAgent.h"
#include "../AIKnowledge/FireObject.h"   // isAttacking
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../Main/AIRandom.h"
#include "../Main/IBotManager.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIUtils/DebugRendering/AIRenderUtil.h"
#include "CommunicationAgent.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFObjectEnvironment.h"
#include "../../../../Math/Frustum.h"
#include "../../../../Math/Mat4.h"
#include <algorithm>
#include "../../../../Core/Debug.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../../../AIInterface/BFKnowledge/IAIObjectPlugIn.h"

#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

const unsigned int kSoldierFlag = 0x200000;

const IBFTimer* timer()
{
    return static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer);
}

// .data 0x4c6ca0, the cosines filled in by the static initialiser: per frustum group (on foot,
// flying) three steps of a field of view and the cosine of the turn that restarts the step.
struct FrustumTweak
{
    float fov;
    float cosine;
};
const FrustumTweak kFrustumTweak[6] = {
    {1.7453292608261108f, static_cast<float>(cos(0.4363323152065277))},
    {1.0471975803375244f, static_cast<float>(cos(0.2617993950843811))},
    {0.5235987901687622f, static_cast<float>(cos(0.13089969754219055))},
    {1.3089969158172607f, static_cast<float>(cos(0.3272492289543152))},
    {0.7853981852531433f, static_cast<float>(cos(0.19634954631328583))},
    {0.2617993950843811f, static_cast<float>(cos(0.06544984877109528))},
};

float distanceSquared(const Vec3& a, const Vec3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

// Nearest first.
struct InfoDistPred
{
    bool operator()(const std::pair<float, unsigned int>& a, const std::pair<float, unsigned int>& b) const
    {
        return a.first < b.first;
    }
};

// vtable 0x004c1d58, 0x1b0e30 (Ghidra). The objects of another side within view distance (the
// neutral ones always so): a soldier only while alive; a unit within its cull distance, or
// within 300 m when the bot is close-quarters (flags 0x10, 0x20).
class SideFilter : public AIObjectPredicate
{
public:
    SideFilter(int side, const Vec3& position, float rangeSquared, bool close)
        : m_side(side), m_position(position), m_rangeSquared(rangeSquared), m_close(close)
    {
    }
    virtual bool predicate(IAIObject* object)
    {
        const int side = object->getSide();
        if (side == 0)
            return distanceSquared(object->getPosition(), m_position) < m_rangeSquared;
        if (side == m_side)
            return false;
        if (object->isOfType(static_cast<InformationType>(0x15))
            && !(object->isPhysical() && 0.0f < object->getPhysical()->getHealth()))
            return false;
        const float distance = distanceSquared(object->getPosition(), m_position);
        if (object->isUnit())
        {
            if (!m_close)
            {
                if (m_rangeSquared <= distance)
                    return false;
                return distance < object->getUnit()->getCullDistanceSqr();
            }
        }
        else if (!m_close)
            return distance < m_rangeSquared;
        return !(m_rangeSquared <= distance) && !(90000.0f <= distance);
    }

    /* +0x04 */ int m_side;
    /* +0x08 */ Vec3 m_position;
    /* +0x14 */ float m_rangeSquared;
    /* +0x18 */ bool m_close;
};

// 0x1a7650 (Ghidra). A soldier's centre: its position, lowered 0.7 m when lying (pose 3).
void soldierHack_getSoldierCentre(Vec3& centre, Vec3& offset, Information* information)
{
    const int pose = static_cast<int>(information->getSoldierPose());
    if (pose == 3)
    {
        offset.x = 0.0f;
        offset.y = -0.7f;
        offset.z = 0.0f;
    }
    else if (pose == 2 || pose == 1)
    {
        offset.x = 0.0f;
        offset.y = 0.0f;
        offset.z = 0.0f;
    }
    else
        Debug(DebugTypeError, "AI/AIDLL/AI/AIBot/BotSenses.cpp", 0xcf, "") << "Unknown pose";
    const Vec3& at = information->getPosition();
    centre.x = offset.x + at.x;
    centre.y = offset.y + at.y;
    centre.z = offset.z + at.z;
}

} // namespace

// Creates the sight frustum and initializes sensing collections for the supplied bot.
BotSenses::BotSenses(BotMain* owner)
    : m_bot(owner)
    // the camera. See the block above.
    , m_frustum(checkPointer(new Frustum()))
    , m_gatherTime(-1000000.0)
    , m_frustumStep(0)
    , m_phase(0)
    , m_sortIndex(0)
    , m_firedStrength(0.0f)
    , m_firedStrengthOwn(0.0f)
{
    // the constructor'S last statement - see the block above. The binary copies the list's head
    // pointer; in source that is the list's own end. The cursor at +80 is deliberately not here.
    m_seenCursor = m_seen.end();
}

// Clears sensed-object records and resets per-life sensing state.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void BotSenses::reset()
{
    for (SensedObjectMap::iterator it = m_sensedObjects.begin(); it != m_sensedObjects.end(); )
    {
        // An object the bot was in contact with gives the contact back.
        if (it->second->m_flag100)
        {
            Information* const information = informationManager->getInformation(it->second->m_handle);
            if (information)
                information->removeContact();
        }
        delete it->second;
        m_sensedObjects.erase(it++);
    }
    m_sensedObjects.clear();

    m_seenNodes.clear();
    m_seen.clear();
    m_seenCursor = m_seen.end();
    m_sortBuffer.clear();

    m_sortIndex = 0;
    m_phase = 0;
    m_frustumStep = 0;
    m_gatherTime = -1000000.0;

    m_heardHandles.clear();
    m_heard.clear();
    // The constructor never did this; the cursor at +80 becomes valid here for the first time.
    m_cursor80 = m_heard.end();

    for (std::list<FireEntry>::iterator it = m_list84.begin(); it != m_list84.end(); ++it)
        delete it->first;
    m_list84.clear();

    m_firedStrength = 0.0f;
    m_firedStrengthOwn = 0.0f;

    m_map100.clear();
}

// Resets senses and deletes the frustum; member containers release their own storage.
BotSenses::~BotSenses()
{
    reset();
    delete m_frustum;
}

// 0x1ac930 (Ghidra).
bool BotSenses::isAttacking(unsigned int handle, double& time) const
{
    std::map<unsigned int, FireObject*>::const_iterator it = m_map100.find(handle);
    if (it == m_map100.end())
    {
        time = -1000.0;
        return false;
    }
    time = it->second->m_time;
    return true;
}

// 0x1ad8f0 (Ghidra). A bot that fired within 1.5 s hears nothing, nor does it hear the dead or
// what neither moves nor carries a weapon. The object carries as far as the louder of its own
// sound sphere (none for a soldier in poses 2 and 3) and that of a weapon it fired within 5 s.
// Within that, the chance runs from the listener's near to its far hearing probability over the
// object's largest sound sphere, and a draw decides.
float BotSenses::hear(unsigned int, Information* information)
{
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    if (!equipment)
        return 0.0f;
    const double now = timer()->getGameTime();
    if (1.5 > now - m_bot->getTimeOfLastFire())
        return 0.0f;
    IPIPhysical* const physical = information->getPhysical();
    if (physical && 0.0f >= physical->getHealth())
        return 0.0f;
    IPIMobile* const mobile = information->getMobile();
    IPIArmament* const armament = information->getArmament();
    if (!armament && !mobile)
        return 0.0f;
    IAIObjectPhysical* const body = information->getAIObject()->getPhysical();
    if (body && 0.0f >= body->getHealth())
        return 0.0f;
    IPIMobile* const listener = equipment->getMobile();
    float moving = mobile ? mobile->getSoundSphereRadius() : 0.0f;
    float firing = 0.0f;
    if (armament)
    {
        WeaponInfo* const weapon = armament->getLastFiredWeapon();
        if (weapon && 5.0 > now - weapon->getTimeOfFiring())
            firing = weapon->getSoundSphereRadius();
    }
    if ((information->getObjectTypeFlags() & kSoldierFlag) != 0)
    {
        const int pose = static_cast<int>(information->getSoldierPose());
        if (pose > 1 && pose <= 3)
            moving = 0.0f;
    }
    const float radius = firing <= moving ? moving : firing;
    // As shipped, the listener's effort never drops below 1, so a listener with a mobile hears
    // whatever is in range; one without never does.
    float effort = 0.0f;
    if (listener)
    {
        effort = listener->getSpeedMagnitude() / listener->getMaxSpeed();
        if (!(effort > 1.0f))
            effort = 1.0f;
    }
    const Vec3& at = information->getAIObject()->getPosition();
    const Vec3& from = equipment->getPosition();
    const float dx = from.x - at.x;
    const float dy = from.y - at.y;
    const float dz = from.z - at.z;
    const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
    if (distance > radius)
        return 0.0f;
    // no null guard on the object's mobile, as shipped
    const float fraction = distance / mobile->getSoundSphereRadiusMax();
    float probability = 1.0f;
    if (listener)
        probability = (1.0f - fraction) * listener->getHearingProbabilityMin()
                      + listener->getHearingProbabilityMax() * fraction;
    const float threshold = (1.0f - effort) * probability;
    const float draw = static_cast<float>(static_cast<double>(AIRandom::m_instance->next())
                                          * 2.3283064365386963e-10);
    return draw > threshold ? probability : 0.0f;
}

// 0x1ad270 (Ghidra). An enemy's sound, while the bot has not fired for 1.5 s; a soldier hears
// soldiers and anything else, anything else only non-soldiers. A new one is put on the grid and
// remembered (a sensed object made here when the bot has none, which forces a replan); one
// already heard is refreshed. Either way its information is renewed for as long as it is loud.
void BotSenses::event_heardObject(unsigned int handle)
{
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    if (!equipment)
        return;
    const double now = timer()->getGameTime();
    if (1.5 > now - m_bot->getTimeOfLastFire() || handle == m_bot->getCurrentEquipment())
        return;
    Information* const target = informationManager->getInformation(handle);
    if (!target)
        return;
    if (target->getSide() == m_bot->getSide())
        return;
    if ((equipment->getObjectTypeFlags() & kSoldierFlag) == 0 && (target->getObjectTypeFlags() & kSoldierFlag) != 0)
        return;
    const float loud = hear(handle, target);
    if (!(loud > 0.0f))
        return;
    const int side = m_bot->getSide();
    const float strength = 0.1f <= loud ? loud : 0.1f;
    if (!m_heardHandles.insert(handle).second)
    {
        // no end guard, as shipped
        SensedObject* const so = m_sensedObjects.find(handle)->second;
        so->m_flags |= SensedObject::kFlag1;
        so->m_int80 = 1;
        so->m_vec28 = target->getPosition();
        so->m_double92 = now;
        so->m_double52 = now;
        // no null guard, as shipped
        informationManager->getInformation(target->getPrimaryObject(side))->setTime(now, strength);
        return;
    }
    SensedObjectMap::iterator it = m_sensedObjects.find(handle);
    target->getAIObject()->addInformationToGrid(target->getSide());
    SensedObject* so;
    if (it == m_sensedObjects.end())
    {
        so = checkPointer(new SensedObject(handle, false, target->getSide() != 0));
        m_sensedObjects.insert(std::make_pair(handle, so));
        m_bot->getSensingAgent()->forceUpdate();
    }
    else
    {
        so = it->second;
        so->m_flags |= SensedObject::kFlag1;
        so->m_int80 = 1;
    }
    so->m_handle = handle;
    so->m_flags |= SensedObject::kFlag1;
    so->m_int80 = 1;
    so->m_double52 = now;
    so->m_vec28 = target->getPosition();
    so->m_double92 = now;
    // no null guard, as shipped
    Information* const primary = informationManager->getInformation(target->getPrimaryObject(side));
    if (primary->setTime(now, strength))
        primary->getAIObject()->addInformationToGrid(side);
    m_heard.push_back(TimedObject(now, so));
}

// 0x1abe60 (Ghidra). While the bot has not fired for 1.5 s, the heard objects come off the front
// of the list one by one, at most once each, until the budget runs out. One gone, or with
// neither mobile nor armament, is forgotten; one still loud renews its information and that of
// everything riding with it; one silent this round is forgotten 30 s after it was last heard.
// The rest go back on the end.
void BotSenses::updateHearing(const double& budget)
{
    const double now = timer()->getGameTime();
    if (1.5 > now - m_bot->getTimeOfLastFire())
        return;
    const double start = timer()->getUpdateTime();
    const double span = budget;
    if (!informationManager->getInformation(m_bot->getCurrentEquipment()))
        return;
    const unsigned int count = static_cast<unsigned int>(m_heard.size());
    const int side = m_bot->getSide();
    for (unsigned int i = 0; i < count && timer()->getUpdateTime() < start + span; ++i)
    {
        SensedObject* const so = m_heard.front().second;
        m_heard.pop_front();
        const unsigned int handle = so->m_handle;
        Information* information = informationManager->getInformation(handle);
        bool forget = !information || (!information->getMobile() && !information->getArmament());
        if (!forget)
        {
            const float loud = hear(handle, information);
            if (loud > 0.0f)
            {
                so->m_flags |= SensedObject::kFlag10;
                so->m_int80 = 0x10;
                so->m_double52 = now;
                // no null guard, as shipped
                Information* const primary = informationManager->getInformation(information->getPrimaryObject(side));
                const float strength = 0.1f <= loud ? loud : 0.1f;
                if (primary->setTime(timer()->getGameTime(), strength))
                    primary->getAIObject()->addInformationToGrid(side);
                int iterator = -1;
                Information* rider = informationManager->getInformation(primary->getFirstSecondaryObject(iterator, side));
                while (rider)
                {
                    rider->setTime(timer()->getGameTime(), loud < 0.1f ? 0.1f : loud);
                    rider = informationManager->getInformation(primary->getSecondaryObject(iterator, side));
                }
            }
            else if (!(so->m_flags & SensedObject::kFlag10))
                forget = now - so->m_double52 > 30.0;
            else
            {
                so->m_flags &= ~SensedObject::kFlag10;
                so->m_double52 = now;
            }
        }
        if (forget)
        {
            m_heardHandles.erase(handle);
            const unsigned int flags = so->m_flags;
            so->m_flags = flags & ~SensedObject::kFlag1;
            if (!(flags & SensedObject::kSeen))
            {
                m_sensedObjects.erase(so->m_handle);
                delete so;
            }
            continue;
        }
        m_heard.push_back(TimedObject(now, so));
    }
}

SensedObject* BotSenses::getSensedObjectFromHandle(unsigned int handle) const
{
    SensedObjectMap::const_iterator it = m_sensedObjects.find(handle);
    if (it == m_sensedObjects.end())
        return 0;
    return it->second;
}

// 0x1adf50 (Ghidra). Whether the object, a primary one, is within 200 m and ahead of the bot's
// primary object.
bool BotSenses::isCloseAndInFront(Information* equipment, IAIObject* object)
{
    Information* const primary =
        informationManager->getInformation(equipment->getPrimaryObject(m_bot->getSide()));
    if (!primary || !object)
        return false;
    const Mat4& frame = primary->getTransformation();
    Information* const target = informationManager->getInformation(object->getInformation(equipment->getSide()));
    if (!target || !target->isPrimary())
        return false;
    const Vec3& at = object->getPosition();
    const float dx = at.x - frame.m[12];
    const float dy = at.y - frame.m[13];
    const float dz = at.z - frame.m[14];
    return dx * dx + dy * dy + dz * dz < 40000.0f && dx * frame.m[8] + dy * frame.m[9] + dz * frame.m[10] > 0.0f;
}

// 0x1acb60 (Ghidra). The object was seen, by the bot itself (self) or by another bot riding with
// it. Its information is renewed; a new one becomes a seen sensed object, first marked as seen
// through another's eyes (flag 8). Seeing it oneself takes the contact (and tells the radio about
// a new enemy); hearing of it from another forces a replan. clear: no see-through surface was in
// the way (else flag 0x40). An object seen through another's eyes and now seen oneself moves to
// the back of the list with its new point.
void BotSenses::event_seenObject(unsigned int handle, const Vec3& localPoint, unsigned int part,
                                 Information* information, bool self, bool clear)
{
    const int side = m_bot->getSide();
    const double now = timer()->getGameTime();
    if (information->setTime(now))
        information->getAIObject()->addInformationToGrid(side);
    std::map<unsigned int, TimedList::iterator>::iterator seen = m_seenNodes.find(handle);
    if (seen == m_seenNodes.end())
    {
        SensedObjectMap::iterator it = m_sensedObjects.find(handle);
        SensedObject* so;
        if (it == m_sensedObjects.end())
        {
            so = checkPointer(new SensedObject(handle, true, information->getSide() != 0));
            so->m_flags |= SensedObject::kFlag8;
            so->m_int80 = 8;
            so->m_double20 = now;
            m_sensedObjects.insert(std::make_pair(handle, so));
        }
        else
            so = it->second;
        so->m_flags |= SensedObject::kSeen;
        so->m_int80 = 2;
        so->m_vec28 = information->getPosition();
        so->m_double92 = now;
        so->m_vec40 = information->getPosition();
        so->m_double84 = now;
        if (self)
        {
            so->m_flags &= ~SensedObject::kFlag8;
            if (!so->m_flag100)
            {
                if (!information->hasContact())
                    m_bot->getCommunicationAgent()->setSeenNewEnemy(information);
                information->addContact();
                so->m_flag100 = true;
            }
        }
        else
            m_bot->getSensingAgent()->forceUpdate();
        if (!clear)
        {
            so->m_flags |= SensedObject::kFlag40;
            so->m_int80 = 0x40;
        }
        so->m_double52 = now;
        so->m_vec4 = localPoint;
        so->m_int16 = static_cast<int>(part);
        m_seen.push_back(TimedObject(now, so));
        m_seenNodes[handle] = --m_seen.end();
        return;
    }
    SensedObject* const so = seen->second->second;
    if (!(so->m_flags & SensedObject::kFlag8) || !self)
        return;
    if (m_seenCursor == seen->second)
        ++m_seenCursor;
    m_seen.erase(seen->second);
    if (!so->m_flag100)
    {
        information->addContact();
        so->m_flag100 = true;
    }
    so->m_vec4 = localPoint;
    so->m_int16 = static_cast<int>(part);
    so->m_vec28 = information->getPosition();
    so->m_double92 = now;
    so->m_vec40 = information->getPosition();
    so->m_flags &= ~SensedObject::kFlag8;
    so->m_double84 = now;
    m_seen.push_back(TimedObject(now, so));
    seen->second = --m_seen.end();
}

// 0x1ac9a0 (Ghidra). A shot noticed by the bot is kept; an enemy shooter counts as seen through
// another's eyes, and its seen record notes when it fired.
void BotSenses::event_projectile(FireObject* fire, unsigned int handle)
{
    m_list84.push_back(FireEntry(fire, handle));
    Information* const shooter = informationManager->getInformation(handle);
    if (!shooter || shooter->getSide() == m_bot->getSide())
        return;
    const unsigned int primaryHandle = shooter->getPrimaryObject(m_bot->getSide());
    Information* const primary = informationManager->getInformation(primaryHandle);
    if (!primary)
        return;
    Vec3 origin;
    origin.x = 0.0f;
    origin.y = 0.0f;
    origin.z = 0.0f;
    event_seenObject(primaryHandle, origin, static_cast<unsigned int>(-1), primary, false, true);
    std::map<unsigned int, TimedList::iterator>::iterator seen = m_seenNodes.find(handle);
    if (seen != m_seenNodes.end())
        seen->second->second->m_double60 = timer()->getGameTime();
}

// 0x1ac460 (Ghidra). Sums the strength of the shots on record, all six battle strengths and the one
// against the bot (an aimed shot, m_flag, counts ten times). A shot older than 60 s over their
// number (at most 5 s) only stays while it is its shooter's latest; the latest shot of every
// shooter is kept in m_map100. The shooters whose latest shot is younger than 30 s over their
// number (at least 2 s) add their strength against the bot to the attack strength.
void BotSenses::updateFireObjects(const double&)
{
    m_firedStrength = 0.0f;
    m_firedStrengthOwn = 0.0f;
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    IPIPhysical* const physical = equipment ? equipment->getPhysical() : 0;
    const int ownType = physical ? physical->getStrengthType() : 0;
    float count = static_cast<float>(m_list84.size());
    if (count < 1.0f)
        count = 1.0f;
    float fade = 60.0f / count;
    if (5.0f < fade)
        fade = 5.0f;
    for (std::list<FireEntry>::iterator it = m_list84.begin(); it != m_list84.end();)
    {
        FireObject* const fire = it->first;
        const float weight = fire->m_flag ? 10.0f : 1.0f;
        for (int i = 0; i < 6; ++i)
            m_firedStrength = fire->getBattleStrength(i) * weight + m_firedStrength;
        if (physical)
            m_firedStrengthOwn = fire->getBattleStrength(ownType) * weight + m_firedStrengthOwn;
        if (timer()->getGameTime() - fire->m_time <= fade)
        {
            std::pair<std::map<unsigned int, FireObject*>::iterator, bool> latest =
                m_map100.insert(std::make_pair(it->second, fire));
            if (latest.second || latest.first->second->m_time < fire->m_time)
                latest.first->second = fire;
            else if (latest.first->second != fire)
            {
                delete fire;
                it = m_list84.erase(it);
                continue;
            }
            ++it;
        }
        else
        {
            std::map<unsigned int, FireObject*>::iterator latest = m_map100.find(it->second);
            if (latest != m_map100.end() && latest->second == fire)
            {
                ++it;
                continue;
            }
            delete fire;
            it = m_list84.erase(it);
        }
    }
    m_attackStrength = 0.0f;
    float shooters = static_cast<float>(m_map100.size());
    if (shooters < 1.0f)
        shooters = 1.0f;
    float window = 30.0f / shooters;
    if (window <= 2.0f)
        window = 2.0f;
    const double now = timer()->getGameTime();
    for (std::map<unsigned int, FireObject*>::iterator it = m_map100.begin(); it != m_map100.end();)
    {
        if (static_cast<double>(window) <= now - it->second->m_time)
            m_map100.erase(it++);
        else
        {
            m_attackStrength = static_cast<float>(it->second->getBattleStrength(ownType) + m_attackStrength);
            ++it;
        }
    }
}

// 0x1a8420 (Ghidra). Sight in phases spread over the ticks, each within the time budget. Every
// call aims the frustum from the player's camera (its width by whether the equipment flies,
// 0x400000). Phase 3 marks the quadrant seen and, when the camera turned past the step's angle,
// restarts the step; phase 0 gathers the living players' objects within view distance of
// another side (SideFilter) that are in the frustum, or close ahead (a non-soldier among
// soldiers) or within three of the bot's radii, with the exposed soldiers they carry; phase 1
// sorts them nearest first; phase 2 looks at each one not already seen, from the eye, at up to
// ten points over its body, and the first point in line of sight makes it seen by every bot in
// the bot's vehicle.
void BotSenses::see(const double& budget)
{
    const double start = timer()->getUpdateTime();
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    m_bot->getName();   // asked and dropped, as shipped
    const Mat4& camera = *static_cast<const Mat4*>(m_bot->getPlayer()->getCameraTransformation());
    m_frustumGroup = (equipment->getObjectTypeFlags() & 0x400000) != 0 ? 1 : 0;
    if (m_phase == 3)
    {
        m_bot->setQuadSeen();
        m_phase = 0;
        const Mat4& previous = m_frustum->getTransformation();
        const float turn = camera.m[9] * previous.m[9] + camera.m[8] * previous.m[8] + camera.m[10] * previous.m[10];
        m_frustumStep = m_frustumStep == 1 ? 2 : (m_frustumStep == 0 ? 1 : 0);
        if (!(kFrustumTweak[m_frustumGroup * 3 + m_frustumStep].cosine < turn))
            m_frustumStep = 0;
    }
    IAIPlayer* const player = m_bot->getPlayer();
    player->getFieldOfView();   // asked and dropped, as shipped
    const float viewDistance = player->getViewDistance();
    m_frustum->setPerspective(kFrustumTweak[m_frustumGroup * 3].fov, 1.0f, 0.01f, viewDistance, camera);
    IAIObjectEnvironment* const environment =
        static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment);

    if (m_phase == 0)
    {
        m_candidates.clear();
        const int side = m_bot->getSide();
        const unsigned int flags = equipment->getObjectTypeFlags();
        const bool close = (flags & 0x10) != 0 || (flags & 0x20) != 0;
        SideFilter filter(side, equipment->getPosition(), viewDistance * viewDistance, close);
        // no null guard, as shipped
        const double reach = static_cast<double>(equipment->getPhysical()->getRadius()) * 3.0;
        environment->getWorldPlayerOccupiedObjects(m_candidates, &filter);
        m_sortBuffer.clear();
        const Vec3& own = equipment->getPosition();
        for (std::vector<IAIObject*>::iterator it = m_candidates.begin(); it != m_candidates.end(); ++it)
        {
            IAIObject* const object = *it;
            Information* const information = informationManager->getInformation(object->getInformation(m_bot->getSide()));
            const float radius = object->getBoundingRadius();
            bool take = m_frustum->inside(object->getPosition(), radius);
            if (!take)
            {
                if ((flags & kSoldierFlag) == 0 && (!information || (information->getObjectTypeFlags() & kSoldierFlag) != 0))
                    take = close && isCloseAndInFront(equipment, object);
                else
                {
                    const Vec3& at = object->getPosition();
                    const float dx = at.x - own.x;
                    const float dy = at.y - own.y;
                    const float dz = at.z - own.z;
                    take = dx * dx + dy * dy + dz * dz <= static_cast<float>(reach * reach);
                }
            }
            if (!take)
                continue;
            m_sortBuffer.push_back(std::make_pair(distanceSquared(own, object->getPosition()),
                                                  object->forceGetInformation(side, false)));
            if (object->isUnit())
            {
                IAIObjectUnit* const unit = object->getUnit();
                if (unit->hasExposedSoldier() && unit->isOccupied())
                {
                    IAIObject* const occupier = unit->getOccupier();
                    m_sortBuffer.push_back(std::make_pair(distanceSquared(own, occupier->getPosition()),
                                                          occupier->forceGetInformation(side, false)));
                }
            }
            // [PLATFORM] Else the binary builds a debug line (0x19d) from the object's name and drops it.
            int iterator;
            for (IAIObject* rider = object->getFirstSecondaryObject(iterator); rider;
                 rider = object->getSecondaryObject(iterator))
            {
                IAIObjectUnit* const unit = rider->getUnit();
                if (!unit || !unit->hasExposedSoldier() || !unit->isOccupied())
                    continue;
                IAIObject* const occupier = unit->getOccupier();
                if (!occupier)
                    continue;
                m_sortBuffer.push_back(std::make_pair(distanceSquared(own, occupier->getPosition()),
                                                      occupier->forceGetInformation(side, false)));
            }
        }
        m_sortIndex = 0;
        m_phase = 1;
        m_gatherTime = timer()->getGameTime();
        if (budget < timer()->getUpdateTime() - start)
            return;
    }
    else if (m_phase != 1 && m_phase != 2)
        return;

    if (m_phase == 1)
    {
        equipment->getPosition();   // asked and dropped, as shipped
        std::sort(m_sortBuffer.begin(), m_sortBuffer.end(), InfoDistPred());
        m_phase = 2;
        if (budget < timer()->getUpdateTime() - start)
            return;
    }

    const Vec3& own = equipment->getPosition();
    std::vector<IAIObject*> ignore;
    ignore.push_back(equipment->getAIObject()->getPrimaryObject());
    ignore.push_back(0);
    ignore.push_back(0);
    IBotManager* const bots = IBotManager::instance;
    AIRenderUtil* const renderer = bots->getRenderBotSensing() ? AIRenderUtil::m_instance : 0;
    const int side = m_bot->getSide();
    double elapsed = timer()->getUpdateTime() - start;
    equipment->getPhysical()->getRadius();   // asked and dropped, as shipped
    while (m_sortIndex < static_cast<int>(m_sortBuffer.size()))
    {
        if (budget <= elapsed)
            return;
        const unsigned int handle = m_sortBuffer[m_sortIndex].second;
        Information* const target = m_seenNodes.find(handle) == m_seenNodes.end()
                                        ? informationManager->getInformation(handle)
                                        : 0;
        if (target && target->isRootObject())
        {
            IAIObject* const object = target->getAIObject();
            Bot* const owner = bots->getBot(object);
            const bool lod = owner && owner->getLodLevel() > 0;
            object->getBoundingRadius();   // asked and dropped, as shipped
            ignore[1] = object;
            setThirdIgnored(ignore, target, side);
            Vec3 eye;
            eye.x = camera.m[12];
            eye.y = camera.m[13] - 0.1f;
            eye.z = camera.m[14];
            const Vec3& at = object->getPosition();
            const float dx = own.x - at.x;
            const float dy = own.y - at.y;
            const float dz = own.z - at.z;
            const float distance = sqrtf(dx * dx + dy * dy + dz * dz);
            // [PLATFORM] With no distance the binary converts 1e14 to an int (0x80000000), which the
            // clamp below turns into one point.
            int points = 1;
            if (0.0f < distance)
            {
                const double share = static_cast<double>(object->getBoundingRadius()) * 30.0 / distance;
                points = share < 1.0 ? 1 : (share > 10.0 ? 10 : static_cast<int>(share));
            }
            unsigned int part = static_cast<unsigned int>(-1);
            const bool soldier = (target->getObjectTypeFlags() & kSoldierFlag) != 0;
            for (int k = 0; k < points; ++k)
            {
                Vec3 point;
                point.x = 0.0f;
                point.y = 0.0f;
                point.z = 0.0f;
                Vec3 local = point;
                if (!soldier && k == 0 && points != 1)
                    point = object->getPosition();
                else if (!soldier)
                    environment->pickRandomSensePosition(point, local, eye, object);
                else
                    environment->pickSoldierRandomSensePosition(point, local, object, part);
                bool clear = false;
                const bool blocked = environment->collideLineWithWorld(eye, point, ignore, lod, 0.8f, true, clear);
                if (renderer)
                    renderer->addSensingLine(eye, point, side, !blocked);
                if (blocked)
                    continue;
                tellRiders(equipment, handle, local, part, target, clear);
                break;
            }
        }
        ++m_sortIndex;
        elapsed = timer()->getUpdateTime() - start;
    }
    m_phase = 3;
    m_sortIndex = 0;
    if (budget < timer()->getUpdateTime() - start)
        return;
    timer()->getUpdateTime();   // asked and dropped, as shipped
}

// The binary writes the third ignored object in place after shrinking the list to two at some
// earlier target; such a write lands past the end and is not seen. Here the write only happens
// while the list still has three entries.
void BotSenses::setThirdIgnored(std::vector<IAIObject*>& ignore, Information* target, int side)
{
    IPIUnit* const unit = target->getUnit();
    IAIObject* third = 0;
    if (unit)
    {
        if ((target->getObjectTypeFlags() & kSoldierFlag) == 0)
        {
            if (unit->hasExposedSoldier())
            {
                Information* const occupier = informationManager->getInformation(unit->getOccupier(side));
                if (occupier)
                    third = occupier->getAIObject();
            }
        }
        else
        {
            Information* const occupied = informationManager->getInformation(unit->getOccupiedObject(side));
            if (occupied && occupied->getUnit() && occupied->getUnit()->hasExposedSoldier())
                third = occupied->getAIObject();
        }
    }
    if (third)
    {
        if (ignore.size() > 2)
            ignore[2] = third;
    }
    else
        ignore.resize(2);
}

// What the bot saw goes to every bot in its vehicle: the one of its primary object and those of
// its secondary objects.
void BotSenses::tellRiders(Information* equipment, unsigned int handle, const Vec3& local, unsigned int part,
                           Information* target, bool clear)
{
    const int side = m_bot->getSide();
    Information* const primary = equipment->isPrimary()
                                     ? equipment
                                     : informationManager->getInformation(equipment->getPrimaryObject(side));
    // no null guard, as shipped
    Bot* bot = IBotManager::instance->getBot(primary->getAIObject());
    if (bot)
        bot->getSensingAgent()->getBotSenses()->event_seenObject(handle, local, part, target, bot == m_bot, clear);
    int iterator = -1;
    for (Information* rider = informationManager->getInformation(primary->getFirstSecondaryObject(iterator, side));
         rider; rider = informationManager->getInformation(primary->getSecondaryObject(iterator, side)))
    {
        bot = IBotManager::instance->getBot(rider->getAIObject());
        if (bot)
            bot->getSensingAgent()->getBotSenses()->event_seenObject(handle, local, part, target, bot == m_bot, clear);
    }
}

// The same, from updateSeeing: the bot itself is skipped and the sighting counts as through
// another's eyes.
void BotSenses::tellOtherRiders(Information* equipment, SensedObject* so, unsigned int part, Information* target,
                                bool clear, const Vec3& eye, const Vec3& local, bool visible)
{
    const int side = m_bot->getSide();
    Information* const primary = equipment->isPrimary()
                                     ? equipment
                                     : informationManager->getInformation(equipment->getPrimaryObject(side));
    // no null guard, as shipped
    Bot* bot = IBotManager::instance->getBot(primary->getAIObject());
    if (bot && bot != m_bot)
    {
        bot->getSensingAgent()->getBotSenses()->event_seenObject(so->m_handle, so->m_vec4, part, target, false, clear);
        if (IBotManager::instance->getRenderBotSensing() && AIRenderUtil::m_instance)
            AIRenderUtil::m_instance->addSensingLine(eye, local, m_bot->getPlayer()->getSide(), visible);
    }
    int iterator = -1;
    for (Information* rider = informationManager->getInformation(primary->getFirstSecondaryObject(iterator, m_bot->getSide()));
         rider; rider = informationManager->getInformation(primary->getSecondaryObject(iterator, m_bot->getSide())))
    {
        bot = IBotManager::instance->getBot(rider->getAIObject());
        if (!bot || bot == m_bot)
            continue;
        bot->getSensingAgent()->getBotSenses()->event_seenObject(so->m_handle, so->m_vec4, part, target, false, clear);
        if (IBotManager::instance->getRenderBotSensing() && AIRenderUtil::m_instance)
            AIRenderUtil::m_instance->addSensingLine(eye, local, m_bot->getPlayer()->getSide(), visible);
    }
}

// The pose test: while the bot fires at the object from another pose than the one it asked for,
// whether the object would be in sight from the asked pose's eye height (BotMain::setPoseStatus).
void BotSenses::testPose(Information* equipment, SensedObject* so, const Vec3& eye, const Vec3& point,
                         const std::vector<IAIObject*>& ignore, bool lod, float radius, bool checkLying)
{
    const int pose = static_cast<int>(equipment->getSoldierPose());
    const int wanted = m_bot->getRequestedSolderPose();
    if (pose == wanted || (checkLying && pose == 4) || m_bot->getFiringTarget() != so->m_handle)
        return;
    Vec3 test = eye;
    if (wanted == 2)
        test.y = pose == 1 ? eye.y - 0.53f : eye.y + 0.38f;
    else if (wanted == 3)
        test.y = eye.y - (pose == 1 ? 0.91f : 0.38f);
    else
        return;
    IAIObjectEnvironment* const environment =
        static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment);
    bool clear = false;
    const bool blocked = environment->collideLineWithWorld(test, point, ignore, lod, radius, true, clear);
    m_bot->setPoseStatus(wanted, so->m_handle, !blocked, clear);
}

// 0x1aa0f0 (Ghidra). Goes round the seen objects from the cursor, for as long as the budget
// allows, checking each one is still in sight. One gone, of the bot's side or dead is forgotten.
// One in the frustum (or for a soldier bot, within five seconds of a shot from it, or within
// twice its radius and three of the bot's), or close ahead, is looked at: a soldier at its centre
// (a bot at reduced detail) or its sense point, another object at its position or the point it
// was last seen at, then at random points. In sight, its information is renewed, it is seen
// oneself, and the bots riding with this one hear of it. Out of sight it is marked seen through
// another's eyes; after 2 s its contact goes, after 60 s (5 s on foot, against aircraft with an
// anti-aircraft weapon or going for a flag; at once for a non-primary object) it is forgotten.
// The rest move to the back of the list.
void BotSenses::updateSeeing(const double& budget)
{
    IBotManager* const bots = IBotManager::instance;
    IAIObjectEnvironment* const environment =
        static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment);
    const bool render = bots->getRenderBotSensing();
    AIRenderUtil* const renderer = render ? AIRenderUtil::m_instance : 0;
    const double start = timer()->getUpdateTime();
    const double now = timer()->getGameTime();
    if (m_seen.empty())
        return;
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    const unsigned int equipmentFlags = equipment->getObjectTypeFlags();
    const float reach = static_cast<float>(static_cast<double>(equipment->getPhysical()->getRadius()) * 3.0);
    std::vector<IAIObject*> ignore;
    ignore.push_back(equipment->getAIObject()->getPrimaryObject());
    ignore.push_back(0);
    ignore.push_back(0);
    const Mat4& camera = *static_cast<const Mat4*>(m_bot->getPlayer()->getCameraTransformation());
    Vec3 eye;
    eye.x = camera.m[12];
    eye.y = camera.m[13] - 0.1f;
    eye.z = camera.m[14];
    // [PLATFORM] The binary stops when the cursor comes back to the node it started at; a node
    // moved to the back reuses its freed address there. The sensed object stands in for it.
    SensedObject* const first = m_seenCursor == m_seen.end() ? 0 : m_seenCursor->second;
    double elapsed;
    do
    {
        if (m_seenCursor == m_seen.end())
            m_seenCursor = m_seen.begin();
        TimedList::iterator node = m_seenCursor;
        ++m_seenCursor;
        SensedObject* const so = node->second;
        Information* const target = informationManager->getInformation(so->m_handle);
        bool forget = !target || !target->getAIObject() || !target->isRootObject();
        if (!forget)
        {
            IPIPhysical* const physical = target->getPhysical();
            forget = target->getSide() == m_bot->getSide() || target->getSide() == 0
                     || (physical && physical->getHealth() <= 0.0f);
        }
        if (!forget)
        {
            IAIObject* const object = target->getAIObject();
            const bool primary = target->isPrimary();
            const bool soldier = (target->getObjectTypeFlags() & kSoldierFlag) != 0;
            const float radius = object->getBoundingRadius();
            const Vec3& at = object->getPosition();
            const float dx = at.x - eye.x;
            const float dy = at.y - eye.y;
            const float dz = at.z - eye.z;
            const float distance = dz * dz + dy * dy + dx * dx;
            bool inView = false;
            if (primary || soldier)
            {
                inView = true;
                const float span = radius + radius + reach;
                if (5.0 < now - so->m_double60 && ((equipmentFlags & kSoldierFlag) == 0 || span * span < distance))
                {
                    IPIUnit* const unit = target->getUnit();
                    if (!m_frustum->inside(object->getPosition(), radius) || (unit && unit->getCullDistanceSqr() < distance))
                        inView = false;
                }
            }
            const bool close = (equipmentFlags & 0x10) != 0 || (equipmentFlags & 0x20) != 0;
            const bool look = inView || (close && isCloseAndInFront(equipment, object));
            bool visible = look;
            Bot* const owner = bots->getBot(object);
            const bool lod = owner && owner->getLodLevel() > 0;
            unsigned int part = static_cast<unsigned int>(-1);
            Vec3 point;
            point.x = 0.0f;
            point.y = 0.0f;
            point.z = 0.0f;
            Vec3 local = point;
            bool clear = false;
            bool drop = false;
            if (look)
            {
                ignore[1] = object;
                writeThirdIgnored(ignore, target, m_bot->getSide());
                if (soldier)
                {
                    if (!lod)
                        visible = false;
                    else
                    {
                        soldierHack_getSoldierCentre(point, local, target);
                        visible = !environment->collideLineWithWorld(eye, point, ignore, lod, 0.75f, true, clear);
                        if (visible)
                            testPose(equipment, so, eye, point, ignore, lod, 0.75f, true);
                    }
                }
                else
                {
                    point = object->getPosition();
                    visible = !environment->collideLineWithWorld(eye, point, ignore, lod, 0.75f, true, clear);
                }
                if (!visible && !lod)
                {
                    if (!soldier)
                    {
                        local = so->m_vec4;
                        const Mat4& frame = object->getTransformation();
                        point.x = frame.m[8] * local.z + frame.m[0] * local.x + frame.m[4] * local.y + frame.m[12];
                        point.y = frame.m[1] * local.x + frame.m[5] * local.y + frame.m[9] * local.z + frame.m[13];
                        point.z = local.y * frame.m[6] + frame.m[2] * local.x + local.z * frame.m[10] + frame.m[14];
                        visible = !environment->collideLineWithWorld(eye, point, ignore, false, 0.75f, true, clear);
                    }
                    else
                    {
                        part = static_cast<unsigned int>(so->m_int16);
                        if (environment->getSoldierSensePosition(point, local, object, part))
                        {
                            visible = !environment->collideLineWithWorld(eye, point, ignore, false, 0.75f, true, clear);
                            if (visible)
                                testPose(equipment, so, eye, point, ignore, false, 0.8f, false);
                        }
                    }
                    if (!visible)
                    {
                        if (soldier)
                        {
                            environment->pickSoldierRandomSensePosition(point, local, object, part);
                            so->m_int16 = static_cast<int>(part);
                        }
                        else
                            environment->pickRandomSensePosition(point, local, eye, object);
                        visible = !environment->collideLineWithWorld(eye, point, ignore, false, 0.75f, true, clear);
                        if (visible)
                            testPose(equipment, so, eye, point, ignore, false, 0.8f, false);
                    }
                }
            }
            if (visible)
            {
                target->setTime(now);
                if (so->m_flags & SensedObject::kFlag8)
                {
                    if (!so->m_flag100)
                    {
                        target->addContact();
                        so->m_flag100 = true;
                    }
                    so->m_flags &= ~SensedObject::kFlag8;
                }
                so->m_vec28 = point;
                so->m_double92 = now;
                so->m_vec40 = point;
                so->m_double84 = now;
                so->m_vec4 = local;
                if (!clear)
                {
                    so->m_flags |= SensedObject::kFlag40;
                    so->m_int80 = 0x40;
                }
                else
                    so->m_flags &= ~SensedObject::kFlag40;
                tellOtherRiders(equipment, so, part, target, clear, eye, local, visible);
            }
            else if (!(so->m_flags & SensedObject::kFlag8))
            {
                so->m_int80 = 8;
                so->m_flags |= SensedObject::kFlag8;
                so->m_double20 = now;
            }
            else
            {
                const float unseen = static_cast<float>(now - so->m_double20);
                Information* const seenInfo = informationManager->getInformation(so->m_handle);
                if (2.0f < unseen && so->m_flag100)
                {
                    so->m_flag100 = false;
                    seenInfo->removeContact();
                    int iterator;
                    for (Information* rider = informationManager->getInformation(seenInfo->getFirstSecondaryObject(iterator, m_bot->getSide()));
                         rider; rider = informationManager->getInformation(seenInfo->getSecondaryObject(iterator, m_bot->getSide())))
                        rider->removeContact();
                }
                // no null guard, as shipped
                Information* const own = informationManager->getInformation(m_bot->getCurrentEquipment());
                IPIArmament* const armament = own->getArmament();
                const bool antiAircraft = armament && (seenInfo->getObjectTypeFlags() & 0x10) != 0 && armament->isAntiAircraft() > 0;
                drop = 60.0f < unseen || !primary || (!own->getMobile() && 5.0f < unseen)
                       || (antiAircraft && 5.0f < unseen) || (m_bot->getIsGoingForFlag() && 5.0f < unseen);
            }
            if (render)
                renderer->addSensingLine(eye, point, m_bot->getSide(), visible);
            if (!drop)
            {
                m_seen.erase(node);
                m_seen.push_back(TimedObject(now, so));
                m_seenNodes[so->m_handle] = --m_seen.end();
            }
            else
                forget = true;
        }
        if (forget)
        {
            m_seen.erase(node);
            m_seenNodes.erase(so->m_handle);
            const unsigned int flags = so->m_flags;
            so->m_flags = flags & ~SensedObject::kSeen;
            if (!(flags & SensedObject::kFlag1))
            {
                m_sensedObjects.erase(so->m_handle);
                delete so;
            }
        }
        elapsed = timer()->getUpdateTime();
    } while ((m_seenCursor == m_seen.end() ? first != 0 : m_seenCursor->second != first)
             && elapsed - start < budget && !m_seen.empty());
}

// The same as setThirdIgnored, for updateSeeing: there the binary only ever writes the third
// entry, and never shrinks the list.
void BotSenses::writeThirdIgnored(std::vector<IAIObject*>& ignore, Information* target, int side)
{
    IPIUnit* const unit = target->getUnit();
    if (!unit)
        return;
    IAIObject* third = 0;
    if ((target->getObjectTypeFlags() & kSoldierFlag) == 0)
    {
        if (unit->hasExposedSoldier())
        {
            Information* const occupier = informationManager->getInformation(unit->getOccupier(side));
            if (occupier)
                third = occupier->getAIObject();
        }
    }
    else
    {
        Information* const occupied = informationManager->getInformation(unit->getOccupiedObject(side));
        if (occupied && occupied->getUnit() && occupied->getUnit()->hasExposedSoldier())
            third = occupied->getAIObject();
    }
    if (third)
        ignore[2] = third;
}

// 0x1aa040 (Ghidra). Hearing gets a tenth of the tick's budget and the shots a third; sight what is
// left of it.
void BotSenses::updateSensing(const double& dt)
{
    const double start = timer()->getUpdateTime();
    const double deadline = start + dt;
    const double hearing = dt * 0.1;
    updateHearing(hearing);
    timer()->getUpdateTime();   // asked and dropped, as shipped
    const double fire = 0.3 * dt;
    updateFireObjects(fire);
    const double seeing = deadline - timer()->getUpdateTime();
    updateSeeing(seeing);
}

// [FRONTIER] Remaining sight, hearing and communication operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

} // namespace ai
} // namespace hfe
} // namespace dice
