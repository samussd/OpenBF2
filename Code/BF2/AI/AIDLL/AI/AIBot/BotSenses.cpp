// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BotSenses operations.

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

} // namespace

// Creates the sight frustum and initializes sensing collections for the supplied bot.
BotSenses::BotSenses(BotMain* owner)
    : m_bot(owner)
    // the camera. See the block above.
    , m_frustum(checkPointer(new Frustum()))
    , m_double12(-1000000.0)
    , m_int32(0)
    , m_int36(0)
    , m_int40(0)
    , m_int88(0)
    , m_int92(0)
{
    // the constructor'S last statement - see the block above. The binary copies the list's head
    // pointer; in source that is the list's own end. The cursor at +80 is deliberately not here.
    m_cursor60 = m_list56.end();
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

    m_map44.clear();
    m_list56.clear();
    m_cursor60 = m_list56.end();
    m_sortBuffer.clear();

    m_int40 = 0;
    m_int36 = 0;
    m_int32 = 0;
    m_double12 = -1000000.0;

    m_heardHandles.clear();
    m_heard.clear();
    // The constructor never did this; the cursor at +80 becomes valid here for the first time.
    m_cursor80 = m_heard.end();

    for (std::list<FireEntry>::iterator it = m_list84.begin(); it != m_list84.end(); ++it)
        delete it->first;
    m_list84.clear();

    m_int88 = 0;
    m_int92 = 0;

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

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void BotSenses::see(const double&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSenses::see");
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void BotSenses::updateSensing(const double&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotSenses::updateSensing");
}

// [FRONTIER] Remaining sight, hearing and communication operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

} // namespace ai
} // namespace hfe
} // namespace dice
