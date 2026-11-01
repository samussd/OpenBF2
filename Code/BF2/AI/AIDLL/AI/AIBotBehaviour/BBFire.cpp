#include "BBFire.h"

#include "CustomCurve.h"
#include "../AIBotBehaviourPattern/BBPFire.h"
#include "../AIBotBehaviourPattern/PlanGenerator.h"
#include "../Main/AIDllFrontier.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/BotMemory.h"
#include "../AIBot/BotNavigation.h"
#include "../AIBot/SensedObject.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIControlInfo.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../AIStrategic/SAI.h"
#include "../AIStrategicEntity/AIStrategicArea.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../AIUtils/VehicleOccupancy.h"
#include "../AIUtils/checkPointer.h"
#include "../Main/AIMain.h"
#include "../Main/AIRandom.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/IAIPathfinding.h"
#include "../Main/IBotManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../../../AIInterface/BFKnowledge/IWeapon.h"
#include "../../../../IO/Input/PlayerInput.h"
#include "../../../../Math/Mat4.h"
#include <algorithm>
#include <cmath>
#include <list>

namespace dice {
namespace hfe {
namespace ai {

namespace {

const unsigned int kSoldierFlag = 0x200000;
const int kReviveToolWeaponType = 3;   // the medic's shock paddles fire too
const int kC4WeaponType = 4;
const int kMedicKit = 5;

// Debug switches, false in .bss and set by nothing read so far.
bool checkOpportunist = false;     // BBFire::calculateUrgency
bool allowMultipleC4 = false;      // BBFire::chooseWeapon
// Non-soldier targets count double. .data, 2.0f.
float enemyStrengthTweak = 2.0f;

double gameTime()
{
    return static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
}

float playerInput(const io::PlayerInput* input, io::PlayerInputMap which)
{
    return (input->mappingFlags >> which) & 1 ? input->inputs[which] : 0.0f;
}

// The knife: slot 0, or slot 5 for a medic.
bool isKnifeSlot(int kit, int slot)
{
    return kit != kMedicKit ? slot == 0 : slot == 5;
}

float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Whether an equipment of that type may stand on the polygon the target last stood on: always
// when the target is of the same type, otherwise as the material there allows.
bool materialAllows(IPIMobile* mobile, const IPIMobile::ValidPos& valid, int equipmentType,
                    IPIUnit* targetUnit)
{
    // no null guard on the unit, as shipped
    if (equipmentType == targetUnit->getEquipmentType())
        return true;
    unsigned char material = 0;
    IAIPathfinding::instance->getMaterial(mobile->getVehicleNumber(), valid.quadTree, valid.polygon,
                                          material);
    return static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)
        ->isValidMaterial(equipmentType, material);
}

} // namespace

BBFire::BBFire(unsigned char behaviourId, unsigned char mode, BBPFire* generator,
               UrgeCurve* urgeCurve, float* weights)
    : BotBehaviour(weights, urgeCurve, getNumberOfBots())
    , m_lastPurge(0.0)
{
    m_behaviourId = behaviourId;
    m_mode = mode;
    m_generator = generator;
    const int numberOfBots = getNumberOfBots();
    m_targets = new int[numberOfBots];
    m_weapons = new int[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
    {
        m_urgency[i] = 0.0f;
        m_weapons[i] = -1;
        m_targets[i] = -1;
    }
    generator->setUrgencyGenerator(this);
    m_curve = new CustomCurve(CustomCurve::modifiedSoftStepFunc, 0.0f, 1.0f, 0.03f);
}

// The target records stay allocated: the map only drops its nodes.
BBFire::~BBFire()
{
    delete[] m_targets;
    delete[] m_weapons;
    delete m_curve;
}

// 0x218040 (Ghidra). The refresh pass keeps the urgency while the target lives. The full pass
// first forgets, once a minute, the records of targets that are gone. A lying soldier, or an
// equipment without a unit, physics, armament or controls, lets the plan go. Every sensed
// enemy the bot has seen is scored: alive, not wrecked or hidden (flags 0x20000, 0x100000,
// 0x800000); one fired at within 20 s only while the two move at 2 m/s together. One heard
// rather than seen (flag 8) counts less the longer ago, and must stand where the bot's
// equipment may go; a stationary weapon must be able to turn its camera onto it. The bot's best
// weapon on it (types 0, 3 and 4; the active one 3 % better) sets its strength, the longest
// range its reach. Aircraft (flag 0x10) are left to anti-aircraft weapons or fought only close
// and slow; a target out of range and out-running the bot counts less, one beyond what the bot
// covers in 5 s not at all. Then the strengths of the target and everything riding with it,
// against the bot's own, weighed by armour and by whether the bot's area contains the target,
// by range, by a random draw on the soft step and by how recently it shot at the bot; a target
// that cannot hurt the bot counts a third. The old target is kept while it scores within 20 %
// of the best; the best weapon on the target is set up; the urgency is the declining slope of
// twice the score times the urgency handed in.
float BBFire::calculateUrgency(Bot* bot, bool flag, float value)
{
    BotMain* const main = static_cast<BotMain*>(bot);
    if (!flag)
    {
        if (!m_generator->isInitiated(bot))
        {
            m_changedTarget[bot->getBotId()] = false;
            m_urgency[bot->getBotId()] = 0.0f;
            return 0.0f;
        }
        if (informationManager->getInformation(static_cast<unsigned int>(m_targets[bot->getBotId()])))
        {
            m_changedTarget[bot->getBotId()] = false;
            bot->addToFireInclination(m_urgency[bot->getBotId()]);
            return m_urgency[bot->getBotId()];
        }
        m_changedTarget[bot->getBotId()] = true;
        m_generator->unInit(bot);
        m_urgency[bot->getBotId()] = 0.0f;
        return 0.0f;
    }

    const double now = gameTime();
    float baseAreaFactor = 0.5f;
    const float* const armour =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getArmourClassValues();
    if (now - m_lastPurge > 60.0)
    {
        for (std::map<unsigned int, BBPFeedback*>::iterator it = m_feedback.begin(); it != m_feedback.end();)
        {
            if (!informationManager->getInformation(it->first))
            {
                delete it->second;
                m_feedback.erase(it++);
            }
            else
                ++it;
        }
        m_lastPurge = now;
    }

    const int side = bot->getSide();
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const Vec3& position = equipment->getPosition();
    const unsigned int equipmentFlags = equipment->getObjectTypeFlags();
    const bool lying = (equipmentFlags & 0x80) == 0 && (equipmentFlags & 0x10) == 0
                       && (equipmentFlags & kSoldierFlag) != 0 && equipment->isPrimary()
                       && static_cast<int>(equipment->getSoldierPose()) == 4;
    IPIUnit* const unit = equipment->getUnit();
    IPIArmament* const armament = equipment->getArmament();
    IPIPhysical* const physical = equipment->getPhysical();
    IPIControlInfo* const controlInfo = equipment->getControlInfo();
    if (lying || !unit || !physical || !armament || !controlInfo)
    {
        m_changedTarget[bot->getBotId()] = true;
        m_generator->unInit(bot);
        m_urgency[bot->getBotId()] = 0.0f;
        return 0.0f;
    }
    // [PLATFORM] The binary asks the unit for its equipment type before the null test above.
    const int equipmentType = unit->getEquipmentType();
    IPIMobile* const mobile = equipment->getMobile();
    const bool exposed = unit->hasExposedSoldier();
    const int ownStrengthType = physical->getStrengthType();
    int exposedStrengthType = 0;
    if (exposed)
    {
        // no null guard, as shipped
        Information* const soldier =
            informationManager->getInformation(bot->getPlayer()->getSoldier()->getInformation(side));
        exposedStrengthType = soldier->getPhysical()->getStrengthType();
    }

    float best = 0.0f;
    float oldScore = 0.0f;
    unsigned int orderedCount = 0;
    const unsigned int id = bot->getBotId();
    const unsigned int oldTarget = static_cast<unsigned int>(m_targets[id]);
    bool oldSeen = false;
    AIStrategicArea* const area = static_cast<AIStrategicArea*>(
        AIStrategicObjectManager::getInstance()->getStrategicObject(
            IAIMain::m_instance->getSAI(side)->getBotOrderedPos(bot->getBotId())));
    if (area && !area->isInside(position))
        baseAreaFactor = 0.85f;
    equipment->getTransformation();   // asked and dropped, as shipped
    m_weapons[bot->getBotId()] = -1;
    float bestDistance = 0.0f;

    const std::map<unsigned int, SensedObject*>& sensed = bot->getSensedObjects();
    for (std::map<unsigned int, SensedObject*>::const_iterator it = sensed.begin(); it != sensed.end(); ++it)
    {
        SensedObject* const so = it->second;
        if (!(so->m_flags & SensedObject::kSeen) || !(so->m_flags & SensedObject::kSided))
            continue;
        Information* const target = informationManager->getInformation(so->m_handle);
        if (!target || target->getSide() == side || target->getSide() == 0)
            continue;
        IPIUnit* const targetUnit = target->getUnit();
        IPIPhysical* const targetPhysical = target->getPhysical();
        if (!targetPhysical)
            continue;
        const unsigned int targetFlags = target->getObjectTypeFlags();
        if ((targetFlags & 0x20000) != 0 || (targetFlags & 0x100000) != 0 || (targetFlags & 0x800000) != 0)
            continue;
        if (0.0f >= targetPhysical->getHealth())
            continue;
        if ((so->m_flags & SensedObject::kFlag20) && so->m_int72 < 0)
            continue;
        IPIMobile* const targetMobile = target->getMobile();
        if (now - getBBPFeedback(so->m_handle)->m_time < 20.0)
        {
            float speed = 0.0f;
            if (mobile)
                speed = mobile->getSpeedMagnitude();
            if (targetMobile)
                speed = speed + targetMobile->getSpeedMagnitude();
            if (2.0f > speed)
                continue;
            main->setHadPrecisionTimeStamp(now);
        }

        float recency = 1.0f;
        double attackedAt = 0.0;
        if (main->isAttacking(so->m_handle, attackedAt))
        {
            const float since = static_cast<float>(now - attackedAt);
            float fraction = since / 30.0f;
            if (0.0f > fraction)
                fraction = 0.0f;
            else if (fraction > 1.0f)
                fraction = 1.0f;
            recency = 1.5f - fraction * 0.5f;
            if (!IBotManager::instance->getBotFromPlayerId(target->getPlayerId())
                && (targetFlags & kSoldierFlag) != 0)
                recency *= 3.0f;
        }

        Vec3 targetPosition;
        targetPosition.x = 0.0f;
        targetPosition.y = 0.0f;
        targetPosition.z = 0.0f;
        if (targetMobile)
            targetMobile->getPositionEstimate(targetPosition);
        else
            targetPosition = target->getPosition();
        Vec3 direction;
        direction.x = targetPosition.x - position.x;
        direction.y = targetPosition.y - position.y;
        direction.z = targetPosition.z - position.z;
        float distance = sqrtf(dot(direction, direction));
        if (distance > 0.0001f)
        {
            const float inverse = 1.0f / distance;
            direction.x = direction.x * inverse;
            direction.y = direction.y * inverse;
            direction.z = inverse * direction.z;
        }
        if (1.0f > distance)
            distance = 1.0f;

        float staleness = 1.0f;
        if (so->m_flags & SensedObject::kFlag8)
        {
            if (!mobile || !targetMobile)
                continue;
            const IPIMobile::ValidPos& valid = targetMobile->getLastValidPosition(mobile->getVehicleNumber());
            if (valid.polygon == static_cast<unsigned int>(-1))
                continue;
            if (!materialAllows(mobile, valid, equipmentType, targetUnit))
                continue;
            const float since = static_cast<float>(now - so->m_double20);
            staleness = 1.0f / (since * 0.05f + 1.5f);
        }
        else if (!mobile)
        {
            Vec3 camera;
            camera.x = 0.0f;
            camera.y = 0.0f;
            camera.z = 0.0f;
            controlInfo->getCameraBasePos(camera);   // asked and dropped, as shipped
            const Mat4& transformation = target->getTransformation();
            const Vec3& local = so->m_vec4;
            Vec3 aim;
            aim.x = transformation.m[8] * local.z + transformation.m[0] * local.x
                    + transformation.m[4] * local.y + transformation.m[12];
            aim.y = transformation.m[1] * local.x + transformation.m[5] * local.y
                    + transformation.m[9] * local.z + transformation.m[13];
            aim.z = local.x * transformation.m[2] + local.y * transformation.m[6]
                    + local.z * transformation.m[10] + transformation.m[14];
            if (((equipmentFlags & 0x10) == 0 || (equipmentFlags & 0x20) != 0)
                && !controlInfo->validateCameraDirectionByPos(aim))
                continue;
        }

        float bestStrength = 0.0f;
        int bestIndex = -1;
        float maxRange = 0.0f;
        WeaponInfo* bestWeapon = 0;
        const std::vector<unsigned int>& weapons = armament->getWeapons();
        for (unsigned int i = 0; i < weapons.size(); ++i)
        {
            WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
            if (!weapon)
                continue;
            const int type = weapon->getWeaponType();
            if (type != 0 && type != kReviveToolWeaponType && type != kC4WeaponType)
                continue;
            if (type == kC4WeaponType && targetMobile)
            {
                const Vec3 facing = equipment->getFaceing();
                if (fabsf(dot(targetMobile->getSpeed(), facing)) > 4.0f)
                    continue;
            }
            int ammo = weapon->getAmmo();
            if (ammo == -1)
                ammo = 0x10000;
            float strength = 0.0f;
            if (ammo > 0)
                strength = static_cast<float>(weapon->getStrength(targetPhysical->getStrengthType()));
            if (static_cast<int>(i) == armament->getActiveWeapon())
                strength *= 1.03f;
            if (strength > bestStrength)
            {
                bestIndex = static_cast<int>(i);
                bestStrength = strength;
                bestWeapon = weapon;
            }
            if (weapon->getMaxRange() > maxRange)
                maxRange = weapon->getMaxRange();
        }
        if (bestStrength == 0.0f)
            continue;

        Vec3 targetVelocity;
        targetVelocity.x = 0.0f;
        targetVelocity.y = 0.0f;
        targetVelocity.z = 0.0f;
        if (targetMobile)
            targetVelocity = targetMobile->getSpeed();
        const float targetSpeed = sqrtf(dot(targetVelocity, targetVelocity));
        float areaFactor = baseAreaFactor;
        if (mobile && (equipmentFlags & 0x10) == 0 && area && !area->isInside(targetPosition))
            areaFactor = areaFactor * 0.85f;

        float reach = 0.0f;
        bool normal = true;
        if (targetFlags & 0x10)
        {
            const int antiAircraft = armament->isAntiAircraft();
            if (antiAircraft == 0)
            {
                if (target->getAltitude() > 5.0f)
                    continue;
                reach = 0.0f;
                normal = !(distance > maxRange * 0.5f) && !(targetSpeed > 5.0f);
            }
            else
            {
                if (checkOpportunist && antiAircraft == 2)
                {
                    const Mat4& transformation = equipment->getTransformation();
                    if (direction.z * transformation.m[6] + direction.x * transformation.m[4]
                            + direction.y * transformation.m[5]
                        > 0.5f)
                        continue;
                }
                reach = 1.0f;
                normal = maxRange * 0.9f <= distance;
            }
        }
        if (normal)
        {
            if (!targetMobile || !(distance > maxRange))
            {
                reach = 1.0f;
                areaFactor = 0.5f;   // as shipped: the area weighting is dropped here
            }
            else
            {
                float ownMaxSpeed = 0.0f;
                Vec3 ownVelocity;
                ownVelocity.x = 0.0f;
                ownVelocity.y = 0.0f;
                ownVelocity.z = 0.0f;
                if (mobile)
                {
                    ownMaxSpeed = mobile->getMaxSpeed();
                    ownVelocity = mobile->getSpeed();
                }
                reach = 0.25f;
                if (targetMobile->getMaxSpeed() - ownMaxSpeed > 5.0f)
                {
                    Vec3 across;   // up cross direction
                    across.x = direction.z;
                    across.y = 0.0f;
                    across.z = -direction.x;
                    const Vec3& velocity = targetMobile->getSpeed();
                    float closing = dot(direction, velocity) - dot(direction, ownVelocity);
                    float lateral = dot(across, velocity) - dot(across, ownVelocity);
                    if (!(0.0f <= closing))
                        closing = 0.0f;
                    if (lateral < 0.0f)
                        lateral = 0.0f;
                    const float speed = sqrtf(dot(velocity, velocity));
                    reach = 0.25f / (speed * 0.5f + closing + closing + 1.0f + lateral + lateral);
                }
            }
        }
        if (distance > maxRange)
        {
            if (!targetMobile || !mobile)
                continue;
            const IPIMobile::ValidPos& valid = targetMobile->getLastValidPosition(mobile->getVehicleNumber());
            if (valid.polygon == static_cast<unsigned int>(-1))
                continue;
            if (!materialAllows(mobile, valid, equipmentType, targetUnit))
                continue;
            if (distance - maxRange > mobile->getMaxSpeed() * 5.0f)
                continue;
        }

        if (!targetUnit || !vehicleOccupied(so->m_handle, side))
            continue;
        float harm = 0.0f;
        float sum = 0.0f;
        unsigned int handle = so->m_handle;
        Information* current = target;
        int iterator = target->getFirstSecondaryObjectIterator();
        const int targetStrengthType = targetPhysical->getStrengthType();
        do
        {
            IPIUnit* strengthUnit = current->getUnit();
            // no null guard, as shipped
            const unsigned int occupied = strengthUnit->getOccupiedObject(side);
            if (occupied != static_cast<unsigned int>(-1))
            {
                Information* const occupiedInfo = informationManager->getInformation(occupied);
                if (occupiedInfo)
                    strengthUnit = occupiedInfo->getUnit();
            }
            harm = harm + strengthUnit->getBattleStrengths()[ownStrengthType];
            bool ordered = false;
            if (bot->isOrderedBehaviour(m_behaviourId) && bot->isTakingOrders())
            {
                std::list<unsigned int>& orders = *bot->getOrderList(m_behaviourId);
                if (orderedCount < orders.size() && std::find(orders.begin(), orders.end(), handle) != orders.end())
                    ordered = true;
            }
            float factor = 1.0f;
            if (ordered)
            {
                ++orderedCount;
                factor = bot->getMemory()->m_float8;
            }
            float strength = strengthUnit->getBattleStrengths()[ownStrengthType];
            if (exposed)
                strength = std::max(strength, strengthUnit->getBattleStrengths()[exposedStrengthType]);
            if ((targetFlags & kSoldierFlag) == 0)
                strength *= enemyStrengthTweak;
            sum = (strength + current->getStrength() + unit->getBattleStrengths()[targetStrengthType])
                      * armour[targetStrengthType] * factor * areaFactor
                  + sum;
            handle = target->getSecondaryObject(iterator, side);
            current = informationManager->getInformation(handle);
        } while (current);

        float rangeFactor = 1.0f - distance / (maxRange * 1.2f);
        if (rangeFactor <= 0.0f)
            rangeFactor = 0.0f;
        const float weighted =
            rangeFactor * bestStrength * staleness * reach * recency * areaFactor * sum;
        const float longest = bestWeapon->getMaxRange();
        const float shortest = bestWeapon->getMinRange();
        const float effective = ((longest - shortest) * 0.1f + shortest) / distance;
        const float draw = static_cast<float>(static_cast<double>(AIRandom::m_instance->next())
                                              * 2.3283064365386963e-10);
        float score = m_curve->calculate(draw, true) * weighted * effective;
        if (!(static_cast<float>(main->getHarmlessThrsh()) < harm))
            score *= 0.33f;
        if (oldTarget == it->first)
        {
            oldScore = score;
            oldSeen = true;
        }
        if (score > best)
        {
            best = score;
            m_targets[bot->getBotId()] = static_cast<int>(it->first);
            m_weapons[bot->getBotId()] = bestIndex;
            bestDistance = distance;
        }
    }

    if (!(best > 0.0f))
    {
        // [UNRESOLVED] The binary also tests a second score here, initialised to 0 and raised by
        // nothing; its branch (targetless set-up and an urgency of its own) is unreachable.
        m_changedTarget[bot->getBotId()] = !oldSeen;
        m_generator->unInit(bot);
        m_urgency[bot->getBotId()] = 0.0f;
        m_targets[bot->getBotId()] = -1;
        return 0.0f;
    }
    float distance = bestDistance;
    bool keepOld = false;
    if (oldSeen)
    {
        oldScore = oldScore * 1.2f;
        keepOld = oldScore > best;
    }
    if (keepOld)
    {
        if (static_cast<unsigned int>(m_targets[bot->getBotId()]) != oldTarget)
        {
            m_targets[bot->getBotId()] = static_cast<int>(oldTarget);
            // no null guard, as shipped
            Information* const old = informationManager->getInformation(oldTarget);
            const Vec3& from = equipment->getPosition();
            const Vec3& to = old->getPosition();
            const float dx = to.x - from.x;
            const float dy = to.y - from.y;
            const float dz = to.z - from.z;
            distance = sqrtf(dx * dx + dy * dy + dz * dz);
        }
    }
    else
        oldSeen = false;
    const int weapon =
        chooseWeapon(bot, static_cast<unsigned int>(m_targets[bot->getBotId()]), distance);
    if (weapon != -1)
        m_weapons[bot->getBotId()] = weapon;
    initializeGenerator(main, m_weapons[bot->getBotId()], false, false);
    if (bot->isOrderedBehaviour(m_behaviourId) && bot->isTakingOrders()
        && bot->getOrderList(m_behaviourId)->empty())
        best = best * bot->getMemory()->m_float8;
    m_changedTarget[bot->getBotId()] = !oldSeen;
    const float factor = 1.0f;             // [UNRESOLVED] a constant factor on the urgency
    m_urgency[bot->getBotId()] = DecleiningSlopeCurve::calculate(best + best) * (factor * value);
    bot->addToFireInclination(m_urgency[bot->getBotId()]);
    return m_urgency[bot->getBotId()];
}

// 0x21a350 (Ghidra). Every weapon of types 0, 3 and 4 that can reach the target (through glass,
// or heard, or flagged 0x40) is scored by its strength against it, scaled down outside its
// range and to a third while its magazine is empty; a reload too slow to finish scores 0.1. A
// soldier closing on a human who provoked it, with its knife slot (0, 5 for a medic) and the
// stamina to run, or any knife within 3 m, takes the knife at once when the path is clear. A
// blast weapon on a soldier from where a lob makes sense, and not too soon after the last throw
// or failed attempt, scores a hundredfold and asks for a static attack.
int BBFire::chooseWeapon(Bot* bot, unsigned int target, float distance)
{
    BotMain* const main = static_cast<BotMain*>(bot);
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const bool soldier = (equipment->getObjectTypeFlags() & kSoldierFlag) != 0;
    IPIArmament* const armament = equipment->getArmament();
    main->setUseStaticAttack(false);
    const double now = gameTime();
    int best = -1;
    float bestScore = 0.0f;
    Information* const targetInfo = informationManager->getInformation(target);
    if (!targetInfo)
        return -1;
    const bool targetSoldier = (targetInfo->getObjectTypeFlags() & kSoldierFlag) != 0;
    IPIPhysical* const targetPhysical = targetInfo->getPhysical();
    IPIMobile* const targetMobile = targetInfo->getMobile();
    if (!targetPhysical)
        return -1;

    bool holdingKnife = false;
    bool ownMagazineEmpty = false;
    if (armament && armament->getActiveWeapon() != -1)
    {
        const int active = armament->getActiveWeapon();
        WeaponInfo* const held = WeaponInfoManager::m_instance->getInformation(armament->getWeapon(active));
        if (!held)
            return -1;
        if (soldier && isKnifeSlot(main->getSpawnKit(), active))
            holdingKnife = true;
        ownMagazineEmpty = held->getWeapon()->getAmmoInMag(0) == 0 && !holdingKnife;
    }
    bool targetMagazineEmpty = false;
    WeaponInfo* targetHeld = 0;
    IPIArmament* const targetArmament = targetInfo->getArmament();
    if (targetArmament && targetArmament->getActiveWeapon() != -1)
    {
        targetHeld = WeaponInfoManager::m_instance->getInformation(
            targetArmament->getWeapon(targetArmament->getActiveWeapon()));
        if (targetHeld)
            targetMagazineEmpty = targetHeld->getWeapon()->getAmmoInMag(0) == 0;
    }
    // Whether the target went for the bot: a human who shot at it (ever, in effect: within
    // 9995 s, and a missing record counts as -1000), or a bot aiming at it.
    bool provoked = false;
    Bot* const targetBot = IBotManager::instance->getBotFromPlayerId(targetInfo->getPlayerId());
    if (!targetBot)
    {
        if (targetHeld)
        {
            double attackedAt = 0.0;
            main->isAttacking(target, attackedAt);
            provoked = now - attackedAt < 9995.0;
        }
    }
    else
    {
        Information* const aim = informationManager->getInformation(targetBot->getFiringTarget());
        if (aim && aim->getPlayerId() == bot->getPlayerId())
            provoked = true;
    }
    bool bothSoldiers = targetSoldier && soldier;
    const bool targetHuman = !IBotManager::instance->getBotFromPlayerId(targetInfo->getPlayerId());
    // no null guards, as shipped
    SensedObject* const sensed = bot->getSensedObjectFromHandle(target);
    const std::vector<unsigned int>& weapons = armament->getWeapons();
    IPIMobile* const mobile = equipment->getMobile();

    for (unsigned int i = 0; i < weapons.size(); ++i)
    {
        WeaponInfo* const weapon = WeaponInfoManager::m_instance->getInformation(weapons[i]);
        if (!weapon)
            continue;
        const int type = weapon->getWeaponType();
        if (type != 0 && type != kC4WeaponType && type != kReviveToolWeaponType)
            continue;
        bool knife = false;
        bool knifeRun = false;
        bool splash = false;
        if (!soldier && targetSoldier && weapon->getWeaponGuidanceType() == 0
            && weapon->getExplosionRadius() != 0.0f)
            splash = true;
        if (!weapon->getFiresThroughTransparent() && !(sensed->m_flags & SensedObject::kFlag8)
            && !(sensed->m_flags & SensedObject::kFlag40))
            continue;
        if (soldier && isKnifeSlot(main->getSpawnKit(), static_cast<int>(i)))
            knife = true;
        if (targetArmament && targetArmament->getActiveWeapon() == 0)
            knife = false;
        if (type == kC4WeaponType)
        {
            if (main->isC4Placed() && !allowMultipleC4)
                continue;
            if (targetMobile)
            {
                const Vec3 facing = equipment->getFaceing();
                if (fabsf(dot(targetMobile->getSpeed(), facing)) > 4.0f)
                    continue;
            }
        }
        if (knife && soldier && targetMobile && mobile)
        {
            const IPIMobile::ValidPos& valid = targetMobile->getLastValidPosition(mobile->getVehicleNumber());
            const Vec3& at = targetInfo->getPosition();
            const float dx = at.x - valid.position.x;
            const float dy = at.y - valid.position.y;
            const float dz = at.z - valid.position.z;
            if (dx * dx + dy * dy + dz * dz > 1.0f
                || main->getBotNavigation()->pointWithinObstacleRadius(targetInfo->getPosition()))
            {
                bothSoldiers = false;
                continue;
            }
        }
        const bool closeKnife = holdingKnife && 10.0f > distance;
        const float stamina = mobile ? mobile->getSprintStamina() : 0.0f;
        if ((stamina > 0.5f || (closeKnife && stamina > 0.0f)) && knife
            && (targetMagazineEmpty || closeKnife) && (ownMagazineEmpty || holdingKnife) && bothSoldiers
            && targetHuman && 20.0f > distance && (provoked || holdingKnife))
            knifeRun = true;

        int ammo = weapon->getAmmo();
        bool staticAttack = false;
        if (ammo == -1)
            ammo = 0x10000;
        if (ammo <= 0 && type != kReviveToolWeaponType && !knifeRun)
            continue;
        float score = static_cast<float>(weapon->getStrength(targetPhysical->getStrengthType()));
        if (score == 0.0f)
            continue;
        if (soldier && targetSoldier && 3.0f > distance && knife)
            knifeRun = true;
        if (knifeRun)
        {
            // no null guards on the two mobiles, as shipped
            const IPIMobile::ValidPos& valid = targetMobile->getLastValidPosition(mobile->getVehicleNumber());
            Vec3 end;
            end.x = 0.0f;
            end.y = 0.0f;
            end.z = 0.0f;
            unsigned int polygon = 0;
            const int ownType = equipment->getUnit()->getEquipmentType();
            if (IAIPathfinding::instance->trace(main->getBotNavigation(), false, valid.position, end,
                                                polygon, ownType, -1, 0.0f, 0, 0))
                return static_cast<int>(i);
        }
        if ((soldier || splash) && targetSoldier && weapon->getWeaponGuidanceType() == 0
            && weapon->getExplosionRadius() != 0.0f)
        {
            // The bot is on its way somewhere: going for a flag, or moving near the order point of
            // a held object whose area it stands in. A target in that area is left alone too.
            bool moving = main->getIsGoingForFlag();
            bool targetInArea = false;
            if (!splash)
            {
                AIStrategicObject* const closest = AIStrategicObjectManager::getInstance()->getClosestObject(
                    equipment->getPosition(), bot->getSide(), false);
                if (closest)
                {
                    // no null guard on the mobile, as shipped
                    const Vec3 orderPos = closest->getOrderPos(mobile->getVehicleNumber(), bot->getSide());
                    const float radius = closest->m_size[bot->getSide()];
                    Vec3 centre;
                    centre.x = 0.0f;
                    centre.y = 0.0f;
                    centre.z = 0.0f;
                    vec2ToVec3XZ(closest->getPosition(bot->getSide()), centre);
                    centre.y = orderPos.y;
                    const float radiusSquared = radius * radius;
                    const float cx = centre.x - orderPos.x;
                    const float cz = centre.z - orderPos.z;
                    if (cx * cx + cz * cz < radiusSquared && closest->getStatus(bot->getSide()) != 0)
                    {
                        const Vec3& own = equipment->getPosition();
                        const float ox = own.x - orderPos.x;
                        const float oy = own.y - orderPos.y;
                        const float oz = own.z - orderPos.z;
                        if (ox * ox + oy * oy + oz * oz < 40.0f * 40.0f)
                        {
                            const io::PlayerInput* const input = bot->getPlayerInput();
                            if (playerInput(input, io::PIThrottle) != 0.0f
                                || playerInput(input, io::PIYaw) != 0.0f
                                || playerInput(input, io::PISprint) != 0.0f)
                                moving = true;
                        }
                    }
                    const Vec3& at = targetInfo->getPosition();
                    const float tx = centre.x - at.x;
                    const float ty = centre.y - at.y;
                    const float tz = centre.z - at.z;
                    if (tx * tx + ty * ty + tz * tz < radiusSquared)
                        targetInArea = true;
                }
            }
            if ((!moving && !targetInArea) || splash)
            {
                const float rise = targetInfo->getPosition().y - equipment->getPosition().y;
                float low = weapon->getMinRange();
                if (!splash)
                    low = rise <= 3.0f ? 10.0f : 5.0f;
                float high = weapon->getMaxRange();
                if (rise > 5.0f && !weapon->getIsThrown())
                    high = rise * 1.25f + high;
                if (main->getBotWantsToSimulateArtillery())
                {
                    low = low - 1.0f;
                    high = 1.0f + high;
                }
                bool lob = false;
                if (main->getSpawnKit() != 1 && !weapon->getIsThrown() && rise > 30.0f)
                    lob = true;
                lob = lob || splash;
                const double window = splash ? 5.0 : 10.0;
                int shots = 1;
                if (!weapon->getIsThrown() && weapon->getBurst())
                    shots = 3;
                const double sinceSensed = now - sensed->m_double84;
                const double sinceThrow = now - main->getTimeOfGrenadeThrown(shots - 1);
                const bool staticFailed = 10.0 > now - main->getTimeOfLastFailedStaticAttack();
                const bool splashFailed = 5.0 > now - main->getTimeOfLastFailedSplashDamageAttack();
                const bool inRange = low <= distance && distance <= high;
                const bool failed = splash ? splashFailed : staticFailed;
                if (inRange && !failed && (sensed->m_flags & SensedObject::kSeen)
                    && ((sensed->m_flags & SensedObject::kFlag8) || lob) && window > sinceSensed
                    && (sinceThrow > 10.0 || splash))
                {
                    staticAttack = true;
                    score = score * 100.0f;
                    main->setUseStaticAttack(true);
                }
            }
        }

        IWeapon* const object = weapon->getWeapon();
        if (object->getAmmoInMag(0) == 0 && weapon->getReloadTime() > 1.9f && weapon->hasAutoReload())
        {
            gameTime();   // asked and dropped, as shipped
            if (object->getWeaponGuidingSystem() == 0 || !object->getProjectile())
                score = 0.1f;
        }
        const float maxRange = weapon->getMaxRange();
        const float minRange = weapon->getMinRange();
        if (!staticAttack)
        {
            if (minRange > distance)
            {
                float under = (minRange - distance) / 5.0f;
                if (under > 1.0f)
                    under = 1.0f;
                score = (0.1f - under * 0.09f) * score;
            }
            else if (distance > maxRange)
            {
                float over = (distance - maxRange) / 5.0f;
                if (over > 1.0f)
                    over = 1.0f;
                score = (1.0f - over * 0.9f) * score;
            }
        }
        if (object->getAmmoInMag(0) == 0)
            score = score * 0.33f;
        if (score > bestScore)
        {
            bestScore = score;
            best = static_cast<int>(i);
        }
    }
    return best;
}

// 0x21a090 (Ghidra).
void BBFire::initializeGenerator(BotMain* bot, int weapon, bool b, bool c)
{
    bot->setFiringTarget(static_cast<unsigned int>(m_targets[bot->getBotId()]));
    static_cast<BBPFire*>(m_generator)
        ->init(bot, static_cast<unsigned int>(m_targets[bot->getBotId()]), weapon, b, c);
}

// 0x21a120 (Ghidra).
void BBFire::initializeGenerator(unsigned int target, const Vec3& position, BotMain* bot, int weapon)
{
    bot->setFiringTarget(target);
    static_cast<BBPFire*>(m_generator)->init(bot, target, position, weapon);
}

// 0x21a180 (Ghidra).
BBFire::BBPFeedback* BBFire::getBBPFeedback(unsigned int handle)
{
    std::map<unsigned int, BBPFeedback*>::iterator it = m_feedback.find(handle);
    if (it != m_feedback.end())
        return it->second;
    BBPFeedback* const feedback = checkPointer(new BBPFeedback);
    m_feedback.insert(std::make_pair(handle, feedback));
    return feedback;
}

} // namespace ai
} // namespace hfe
} // namespace dice
