// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BAPEntries operations.

#include "BAPEntries.h"

#include "InfanteryControl.h"
#include "MouseControl.h"
#include "PlanInterpreter.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/SensingAgent.h"
#include "../AIBotActionPlan/BAPAMoveTo.h"
#include "../AIBotActionPlan/BAPALook.h"
#include "../AIBotActionPlan/BAPAResetControls.h"
#include "../AIBotActionPlan/BAPASoldierPose.h"
#include "../AIBotActionPlan/BAPATrigger.h"
#include "../AIBotActionPlan/BAPInfoWrapper.h"
#include "../AIBotActionPlan/BAPCondition.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIControlInfo.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/IBotManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFGameEffector.h"
#include "../../../AIInterface/BFKnowledge/BFObjectEnvironment.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../../../AIInterface/BFKnowledge/IAIObjectPlugIn.h"
#include "../../../../Math/Mat4.h"
#include <cfloat>
#include <cmath>
#include "../Main/AIDllFrontier.h"
#include "../../../../IO/Input/PlayerInput.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

// A soldier close to a player runs on physics and inputs.
void wakePhysics(Information* equipment)
{
    IAIObjectPhysical* const physical = equipment->getAIObject()->getPhysical();
    if (!physical->hasPhysicsEnabled())
        physical->enablePhysics(equipment->getSide());
}

// One soldier further off is placed by hand and has no physics.
void sleepPhysics(IAIObject* object)
{
    IAIObjectPhysical* const physical = object->getPhysical();
    if (physical->hasPhysicsEnabled())
        physical->disablePhysics(object->getSide());
}

// The speed of a soldier placed by hand in a pose: half crouched, a quarter lying, none otherwise.
float poseSpeed(float speed, int pose)
{
    if (pose == SPCrouching)
        return speed * 0.5f;
    if (pose == SPLying)
        return speed * 0.25f;
    return pose == SPStanding ? speed : 0.0f;
}

void normalizeOrZero(Vec3& v)
{
    const float length2 = v.x * v.x + v.y * v.y + v.z * v.z;
    if (std::fabs(length2 - 1.0f) < 1.1920929e-07f)
        return;
    if (std::fabs(length2) < 1.1920929e-07f)
    {
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        return;
    }
    const float inverse = 1.0f / std::sqrt(length2);
    v.x = v.x * inverse;
    v.y = v.y * inverse;
    v.z = inverse * v.z;
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
    Vec3 out;
    out.x = a.y * b.z - a.z * b.y;
    out.y = a.z * b.x - a.x * b.z;
    out.z = a.x * b.y - a.y * b.x;
    return out;
}

// Sets a hand-placed soldier down at `position` (its y is replaced), facing `forward`: on the ground
// when near it or under it, otherwise dropping by the slope. A lying soldier lies half a metre lower
// along the ground; the others stand upright. The position move leaves the side vector as it comes.
void placeSoldier(IAIObject* object, IAIObjectMobile* mobile, Mat4& matrix, Vec3 position, Vec3 forward, int pose,
                  float y, bool unitRight, int side)
{
    float height = 0.0f;
    float slope = 0.0f;
    Vec3 normal;
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = 0.0f;
    static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment)
        ->getHeightAndNormalForObject(object, position, height, normal, slope);
    if (y <= height || y - height <= slope + slope)
        position.y = mobile->getLodHeight() + height;
    else
        position.y = y - slope;
    Vec3 right;
    if (pose == SPLying)
    {
        position.y = position.y - 0.5f;
        right = cross(normal, forward);
        normalizeOrZero(right);
        forward = cross(right, normal);
        normalizeOrZero(forward);
    }
    else
    {
        normal.x = 0.0f;
        normal.y = 1.0f;
        normal.z = 0.0f;
        right = cross(normal, forward);
        if (unitRight)
            normalizeOrZero(right);
    }
    matrix.m[0] = right.x;
    matrix.m[1] = right.y;
    matrix.m[2] = right.z;
    matrix.m[4] = normal.x;
    matrix.m[5] = normal.y;
    matrix.m[6] = normal.z;
    matrix.m[8] = forward.x;
    matrix.m[9] = forward.y;
    matrix.m[10] = forward.z;
    matrix.m[12] = position.x;
    matrix.m[13] = position.y;
    matrix.m[14] = position.z;
    object->setTransformation(matrix, side);
}

} // namespace

// Action-plan primitives return their statement type for registry dispatch.

// Empty body; see BAPEntry.h.
BAPEntry::~BAPEntry()
{
}

EntryTankMoveTo::EntryTankMoveTo()
{
}

EntryTankMoveTo::~EntryTankMoveTo()
{
}

int EntryTankMoveTo::getTargetId()
{
    return 7;
}

bool EntryTankMoveTo::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryTankMoveTo::execute");
}


EntryTankMoveToObject::EntryTankMoveToObject()
{
}

EntryTankMoveToObject::~EntryTankMoveToObject()
{
}

int EntryTankMoveToObject::getTargetId()
{
    return 9;
}

bool EntryTankMoveToObject::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryTankMoveToObject::execute");
}


EntryTankMoveToDirection::EntryTankMoveToDirection()
{
}

EntryTankMoveToDirection::~EntryTankMoveToDirection()
{
}

int EntryTankMoveToDirection::getTargetId()
{
    return 8;
}

bool EntryTankMoveToDirection::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryTankMoveToDirection::execute");
}


EntryTankResetControls::EntryTankResetControls()
{
}

EntryTankResetControls::~EntryTankResetControls()
{
}

int EntryTankResetControls::getTargetId()
{
    return 14;
}

bool EntryTankResetControls::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryTankResetControls::execute");
}


EntryTankTurnTo::EntryTankTurnTo()
{
}

EntryTankTurnTo::~EntryTankTurnTo()
{
}

int EntryTankTurnTo::getTargetId()
{
    return 24;
}

bool EntryTankTurnTo::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryTankTurnTo::execute");
}

void EntryTankTurnTo::getCentreOfFacingSide(const Vec2&, const Vec2&, const Vec2&,
                                           const Vec2&, Vec2&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryTankTurnTo::getCentreOfFacingSide");
}


EntryCarMoveTo::EntryCarMoveTo()
{
}

EntryCarMoveTo::~EntryCarMoveTo()
{
}

int EntryCarMoveTo::getTargetId()
{
    return 7;
}

bool EntryCarMoveTo::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryCarMoveTo::execute");
}


EntryCarMoveToDirection::EntryCarMoveToDirection()
{
}

EntryCarMoveToDirection::~EntryCarMoveToDirection()
{
}

int EntryCarMoveToDirection::getTargetId()
{
    return 8;
}

// A retail stub - see the block above. , and all it does is succeed.
bool EntryCarMoveToDirection::execute(BotScript *, Bot *, bool&, bool&)
{
    return true;
}


EntryCarMoveToObject::EntryCarMoveToObject()
{
}

EntryCarMoveToObject::~EntryCarMoveToObject()
{
}

int EntryCarMoveToObject::getTargetId()
{
    return 9;
}

bool EntryCarMoveToObject::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryCarMoveToObject::execute");
}


EntryCarResetControls::EntryCarResetControls()
{
}

EntryCarResetControls::~EntryCarResetControls()
{
}

int EntryCarResetControls::getTargetId()
{
    return 14;
}

bool EntryCarResetControls::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryCarResetControls::execute");
}


EntryPlaneMoveTo::EntryPlaneMoveTo()
{
}

EntryPlaneMoveTo::~EntryPlaneMoveTo()
{
}

int EntryPlaneMoveTo::getTargetId()
{
    return 7;
}

bool EntryPlaneMoveTo::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryPlaneMoveTo::execute");
}


EntryPlaneMoveToDirection::EntryPlaneMoveToDirection()
{
}

EntryPlaneMoveToDirection::~EntryPlaneMoveToDirection()
{
}

int EntryPlaneMoveToDirection::getTargetId()
{
    return 8;
}

bool EntryPlaneMoveToDirection::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryPlaneMoveToDirection::execute");
}


EntryPlaneMoveToObject::EntryPlaneMoveToObject()
{
}

EntryPlaneMoveToObject::~EntryPlaneMoveToObject()
{
}

int EntryPlaneMoveToObject::getTargetId()
{
    return 9;
}

bool EntryPlaneMoveToObject::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryPlaneMoveToObject::execute");
}


EntryPlaneAimAt::EntryPlaneAimAt()
{
}

EntryPlaneAimAt::~EntryPlaneAimAt()
{
}

int EntryPlaneAimAt::getTargetId()
{
    return 6;
}

bool EntryPlaneAimAt::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryPlaneAimAt::execute");
}


EntryPlaneResetControls::EntryPlaneResetControls()
{
}

EntryPlaneResetControls::~EntryPlaneResetControls()
{
}

int EntryPlaneResetControls::getTargetId()
{
    return 14;
}

bool EntryPlaneResetControls::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryPlaneResetControls::execute");
}


EntryBoatMoveTo::EntryBoatMoveTo()
{
}

EntryBoatMoveTo::~EntryBoatMoveTo()
{
}

int EntryBoatMoveTo::getTargetId()
{
    return 7;
}

bool EntryBoatMoveTo::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryBoatMoveTo::execute");
}


EntryBoatMoveToObject::EntryBoatMoveToObject()
{
}

EntryBoatMoveToObject::~EntryBoatMoveToObject()
{
}

int EntryBoatMoveToObject::getTargetId()
{
    return 9;
}

bool EntryBoatMoveToObject::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryBoatMoveToObject::execute");
}


EntryBoatMoveToDirection::EntryBoatMoveToDirection()
{
}

EntryBoatMoveToDirection::~EntryBoatMoveToDirection()
{
}

int EntryBoatMoveToDirection::getTargetId()
{
    return 8;
}

bool EntryBoatMoveToDirection::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryBoatMoveToDirection::execute");
}


EntryBoatResetControls::EntryBoatResetControls()
{
}

EntryBoatResetControls::~EntryBoatResetControls()
{
}

int EntryBoatResetControls::getTargetId()
{
    return 14;
}

bool EntryBoatResetControls::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryBoatResetControls::execute");
}


EntryInfanteryMoveTo::EntryInfanteryMoveTo()
{
}

EntryInfanteryMoveTo::~EntryInfanteryMoveTo()
{
}

int EntryInfanteryMoveTo::getTargetId()
{
    return 7;
}

// 0x2c7aa0 (Ghidra). One step of a move to a position: the inputs for the direction getTargetDir
// gives. No move (the path is over, or an obstacle takes the step) lets go of the movement inputs.
bool EntryInfanteryMoveTo::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPAMoveTo* const move = static_cast<BAPAMoveTo*>(script);
    BotMain* const main = static_cast<BotMain*>(bot);
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    IPIMobile* const mobile = equipment ? equipment->getMobile() : 0;
    if (!mobile)
    {
        done = true;
        ok = false;
        return true;
    }
    wakePhysics(equipment);
    if (bot->isFireAllowed() && move->isFinished(bot))
    {
        done = true;
        ok = true;
        return true;
    }
    done = false;
    Vec2 point;
    point.x = 0.0f;
    point.y = 0.0f;
    Vec2 direction = point;
    bool stopped = false;
    ok = move->update(point, done, stopped, 0.0f);
    if (ok)
    {
        if (stopped || done)
        {
            ok = infanteryResetControls(bot, main->getPlayerInput(), true, false, false);
            return true;
        }
        if (!move->getTargetDir(bot, point, direction, 0, false))
        {
            bot->getSensingAgent()->setMoveDirection(direction);
            removeZoom(bot);
            infanteryControlThrottle(bot, main->getPlayerInput(), equipment->getControlInfo(), mobile->getMaxSpeed(),
                                     move->getIsSprinting());
            if (equipment->getSoldierPose() != SPSwimming)
                bot->requestSoldierPose(SPStanding, true);
            return true;
        }
    }
    ok = infanteryResetControls(bot, main->getPlayerInput(), true, false, false);
    done = true;
    return true;
}


EntryInfanteryMoveToObject::EntryInfanteryMoveToObject()
{
}

EntryInfanteryMoveToObject::~EntryInfanteryMoveToObject()
{
}

int EntryInfanteryMoveToObject::getTargetId()
{
    return 9;
}

// 0x2c80d0 (Ghidra). A step of a move to an object, as EntryInfanteryMoveTo. Under 1.5 s since the
// failed sky attack the failed static attack time moves on; after it a wish to simulate artillery
// is dropped. A move that is stuck kills the bot.
bool EntryInfanteryMoveToObject::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPAMoveTo* const move = static_cast<BAPAMoveTo*>(script);
    BotMain* const main = static_cast<BotMain*>(bot);
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    IPIMobile* const mobile = equipment ? equipment->getMobile() : 0;
    if (!mobile)
    {
        done = true;
        ok = false;
        return true;
    }
    wakePhysics(equipment);
    if (!informationManager->getInformation(move->getObject()))
    {
        done = true;
        ok = false;
        return true;
    }
    if (bot->isFireAllowed() && move->isFinished(bot))
    {
        done = true;
        ok = true;
        return true;
    }
    done = false;
    const double now = static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    if (1.5f <= now - main->getTimeOfLastFailedStaticSkyAttack())
    {
        if (main->getBotWantsToSimulateArtillery())
        {
            main->setTimeOfLastFailedStaticSkyAttack(now);
            main->setBotWantsToSimulateArtillery(false);
        }
    }
    else
        main->setTimeOfLastFailedStaticAttack(now);
    Vec2 point;
    point.x = 0.0f;
    point.y = 0.0f;
    Vec2 direction = point;
    bool stopped = false;
    ok = move->update(point, done, stopped, 0.0f);
    if (ok)
    {
        if (stopped || done)
        {
            ok = infanteryResetControls(bot, main->getPlayerInput(), true, false, false);
            return true;
        }
        Information* const target = informationManager->getInformation(move->getObject());
        if (!move->getTargetDir(bot, point, direction, target, move->getIsMovingAway()))
        {
            bot->getSensingAgent()->setMoveDirection(direction);
            removeZoom(bot);
            infanteryControlThrottle(bot, main->getPlayerInput(), equipment->getControlInfo(), mobile->getMaxSpeed(),
                                     move->getIsSprinting());
            if (equipment->getSoldierPose() != SPSwimming)
                bot->requestSoldierPose(SPStanding, true);
            if (move->isStuck())
                static_cast<IAIGameEffector*>(CommonSingletonServer::m_iaiGameEffector)->suicide(bot->getPlayer());
            return true;
        }
    }
    ok = infanteryResetControls(bot, main->getPlayerInput(), true, false, false);
    done = true;
    return true;
}


EntryInfanteryMoveToDirection::EntryInfanteryMoveToDirection()
{
}

EntryInfanteryMoveToDirection::~EntryInfanteryMoveToDirection()
{
}

int EntryInfanteryMoveToDirection::getTargetId()
{
    return 8;
}

// A retail stub - see the block above. , and all it does is succeed.
bool EntryInfanteryMoveToDirection::execute(BotScript *, Bot *, bool&, bool&)
{
    return true;
}


EntryInfanteryResetControls::EntryInfanteryResetControls()
{
}

EntryInfanteryResetControls::~EntryInfanteryResetControls()
{
}

int EntryInfanteryResetControls::getTargetId()
{
    return 14;
}

// A statement with a single input releases that input; otherwise the soldier's control groups.
bool EntryInfanteryResetControls::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPAResetControls* const reset = static_cast<BAPAResetControls*>(script);
    if (!reset->hasSingleInput())
    {
        infanteryResetControls(bot, static_cast<BotMain*>(bot)->getPlayerInput(), reset->m_movement,
                               reset->m_look, reset->m_actions);
        done = true;
        ok = true;
        return true;
    }
    static_cast<BotMain*>(bot)->getPlayerInput()->setInput(
        static_cast<io::PlayerInputMap>(reset->getInput()), 0.0f);
    ok = true;
    done = true;
    return true;
}


EntryTrigger::EntryTrigger()
{
}

EntryTrigger::~EntryTrigger()
{
}

int EntryTrigger::getTargetId()
{
    return 11;
}

// Before a shot: settles the pose the weapon wants and how well the bot aims with it.
static void prepareFiring(Bot* bot, Information* equipment, IPIArmament* armament, WeaponInfo* weapon,
                          bool moving)
{
    const bool soldier = (equipment->getObjectTypeFlags() & 0x200000u) != 0;
    if (!moving && weapon->getWeaponType() != 4)
    {
        int pose = 1;
        if (soldier)
        {
            pose = weapon->getFiringPose();
            if (armament->getActiveWeapon() == 0
                || (bot->getFiringTarget() != 0xFFFFFFFFu && weapon->getWeaponType() == 3))
            {
                Vec2 own = {};
                Vec2 other = {};
                vec3ToVec2XZ(equipment->getPosition(), own);
                Information* const target = informationManager->getInformation(bot->getFiringTarget());
                if (target)
                {
                    vec3ToVec2XZ(target->getPosition(), other);
                    const float dx = other.x - own.x;
                    const float dy = other.y - own.y;
                    if (dy * dy + dx * dx < 1.0f && target->getSoldierPose() == 3)
                        pose = 2;
                }
            }
        }
        const int current = equipment->getSoldierPose();
        if (current != 4 && current != pose)
            bot->requestSoldierPose(pose, false);
    }

    Information* const target = informationManager->getInformation(bot->getFiringTarget());
    const float botSkill = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getBotSkill();
    const bool human = target && target->getAIObject()
        && !IBotManager::instance->getBot(target->getAIObject())
        && target->getUnit() && target->getUnit()->isOccupied();

    float skill = 1.0f;
    if (!weapon->getIsThrown())
    {
        if (weapon->getExplosionRadius() == 0.0f)
            skill = human ? botSkill : 1.0f - (1.0f - botSkill) * 0.5f;
        else
            skill = 1.0f - (1.0f - botSkill) * (soldier ? 0.4f : 0.8f);
    }
    if (weapon->getWeaponGuidanceType() != 0)
        skill = 1.0f;

    float scale = 1.0f;
    const unsigned int flags = equipment->getObjectTypeFlags();
    if ((flags & 0x10u) == 0)
    {
        if (!target)
            return;
        if ((target->getObjectTypeFlags() & 0x10u) != 0 && !equipment->isPrimary()
            && (flags & 0x80u) == 0)
            scale = 3.0f;
    }
    weapon->setBotSkill(skill, bot->getFiringTargetTime(), scale);
}

// Presses or lets go of the action's input. A zoom waits until the bot stops, a weapon change
// ends as soon as the weapon is in hand, and firing first settles pose and aim.
bool EntryTrigger::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPATrigger* const action = static_cast<BAPATrigger*>(script);
    const io::PlayerInputMap key = action->m_input;
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    IPIArmament* const armament = equipment->getArmament();
    WeaponInfo* weapon = 0;
    if (armament && armament->getActiveWeapon() != -1)
        weapon = WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));

    io::PlayerInput* const input = static_cast<BotMain*>(bot)->getPlayerInput();
    const bool moving = input->getInput(static_cast<io::PlayerInputMap>(3)) != 0.0f
                     || input->getInput(io::PIYaw) != 0.0f
                     || input->getInput(io::PISprint) != 0.0f;
    const bool firing = key == io::PIFire || static_cast<int>(key) == 0x1F;
    const bool allowed = !(firing && static_cast<BotMain*>(bot)->botInPoseTransition());

    if (weapon)
    {
        if (static_cast<unsigned int>(key) - 14u < 8u)
        {
            if (weapon->getWeaponActivate() == static_cast<int>(key))
            {
                ok = true;
                done = true;
                return true;
            }
            if (action->m_bool20)
            {
                ok = true;
                done = false;
                return true;
            }
        }
        else if (weapon->getWeaponZoom() == static_cast<int>(key))
        {
            if (moving)
                return true;
        }
        else if (firing)
        {
            prepareFiring(bot, equipment, armament, weapon, moving);
        }
    }

    if (static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime() < action->m_startTime)
        return true;

    if (action->m_breakCondition && action->m_breakCondition->evaluate(bot))
    {
        if (input->getInput(key) != 0.0f)
            input->setInput(key, 0.0f);
        action->m_bool20 = false;
        ok = true;
        done = true;
        return true;
    }

    if (action->m_bool20 && action->m_bool2C)
    {
        input->setInput(key, 0.0f);
        action->m_bool20 = false;
        ok = true;
        done = true;
        return true;
    }

    if (allowed && action->m_runCondition->evaluate(bot))
    {
        input->setInput(key, 1.0f);
        action->m_bool20 = true;
        ok = true;
        done = !action->m_bool2C;
        return true;
    }

    input->setInput(key, 0.0f);
    action->m_bool20 = false;
    ok = true;
    done = false;
    return true;
}


EntryTriggerContinously::EntryTriggerContinously()
{
}

EntryTriggerContinously::~EntryTriggerContinously()
{
}

int EntryTriggerContinously::getTargetId()
{
    return 12;
}

bool EntryTriggerContinously::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryTriggerContinously::execute");
}


EntryKeyTurretAimAt::EntryKeyTurretAimAt()
{
}

EntryKeyTurretAimAt::~EntryKeyTurretAimAt()
{
}

int EntryKeyTurretAimAt::getTargetId()
{
    return 6;
}

bool EntryKeyTurretAimAt::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryKeyTurretAimAt::execute");
}


EntryMouseTurretAimAt::EntryMouseTurretAimAt()
{
}

EntryMouseTurretAimAt::~EntryMouseTurretAimAt()
{
}

int EntryMouseTurretAimAt::getTargetId()
{
    return 6;
}

bool EntryMouseTurretAimAt::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryMouseTurretAimAt::execute");
}


EntryMouseTurretLookAt::EntryMouseTurretLookAt()
{
}

EntryMouseTurretLookAt::~EntryMouseTurretLookAt()
{
}

int EntryMouseTurretLookAt::getTargetId()
{
    return 23;
}

// Holds the sensing override while it looks. The look's yaw error is reset to pi whenever it ends
// or cannot run; otherwise the mouse controller turns the view and the look keeps the errors and
// the camera's forward for the next tick. Never done by itself: the condition ends it.
bool EntryMouseTurretLookAt::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPALook* const look = static_cast<BAPALook*>(script);
    BotMain* const botMain = static_cast<BotMain*>(bot);
    botMain->getSensingAgent()->setOverride(true);

    if (look->m_until->evaluate(bot))
    {
        done = true;
        ok = true;
        look->m_float28 = 3.1415927f;
        return true;
    }
    done = false;

    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    if (!equipment)
    {
        done = true;
        ok = false;
        look->m_float28 = 3.1415927f;
        return true;
    }

    Vec3 direction = look->getLookDirection(ok);
    const float lengthSquared =
        direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;
    if (!(std::fabs(lengthSquared - 1.0f) < FLT_EPSILON))
    {
        if (std::fabs(lengthSquared) < FLT_EPSILON)
        {
            direction.x = 0.0f;
            direction.y = 0.0f;
            direction.z = 0.0f;
        }
        else
        {
            const float scale = 1.0f / std::sqrt(lengthSquared);
            direction.x *= scale;
            direction.y *= scale;
            direction.z *= scale;
        }
    }
    if (!ok)
    {
        done = false;
        look->m_float28 = 3.1415927f;
        return true;
    }

    IPIControlInfo* const control = equipment->getControlInfo();
    ok = true;
    io::PlayerInput* const input = botMain->getPlayerInput();
    const float allowedDeviation = look->m_float24;
    const float previousYawError = look->m_float28;
    const float previousPitchError = look->m_float32;
    float pitchError;
    float yawError;
    mouseControlLookAtDirection(bot, direction,
        *static_cast<const Mat4*>(bot->getPlayer()->getCameraTransformation()),
        pitchError, previousPitchError, yawError, previousYawError, allowedDeviation, control,
        input, 0.0f, 0.0f, 0.0f);
    look->m_float28 = yawError;
    look->m_float32 = pitchError;

    const Mat4& camera = *static_cast<const Mat4*>(bot->getPlayer()->getCameraTransformation());
    look->m_vec36.x = camera.m[8];
    look->m_vec36.y = camera.m[9];
    look->m_vec36.z = camera.m[10];
    return true;
}


EntrySense::EntrySense()
{
}

EntrySense::~EntrySense()
{
}

int EntrySense::getTargetId()
{
    return 20;
}

bool EntrySense::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntrySense::execute");
}


EntrySoldierPose::EntrySoldierPose()
{
}

EntrySoldierPose::~EntrySoldierPose()
{
}

int EntrySoldierPose::getTargetId()
{
    return 26;
}

// A soldier that is turning, moving or sprinting is asked to stand; otherwise the statement's
// normal pose. The entry finishes once the soldier holds that pose. A swimming soldier is left
// alone. Retail does not guard the equipment.
bool EntrySoldierPose::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPASoldierPose* const pose = static_cast<BAPASoldierPose*>(script);
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    SoldierPose wanted = pose->getNormalPose();
    const io::PlayerInput* const input = static_cast<BotMain*>(bot)->getPlayerInput();
    const unsigned long long mapped = input->mappingFlags;
    const float throttle = (mapped >> io::PIThrottle) & 1 ? input->inputs[io::PIThrottle] : 0.0f;
    const float yaw = (mapped >> io::PIYaw) & 1 ? input->inputs[io::PIYaw] : 0.0f;
    const float sprint = (mapped >> io::PISprint) & 1 ? input->inputs[io::PISprint] : 0.0f;
    if (throttle != 0.0f || yaw != 0.0f || sprint != 0.0f)
        wanted = SPStanding;

    if (equipment->getSoldierPose() == wanted)
    {
        ok = true;
        done = true;
        return true;
    }
    const SoldierPose current = equipment->getSoldierPose();
    if (current != SPSwimming && current != wanted)
        bot->requestSoldierPose(wanted, pose->m_bool48);
    ok = true;
    done = false;
    return true;
}


EntryMoveToMedium::EntryMoveToMedium()
{
}

EntryMoveToMedium::~EntryMoveToMedium()
{
}

int EntryMoveToMedium::getTargetId()
{
    return 7;
}

bool EntryMoveToMedium::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryMoveToMedium::execute");
}


EntryMoveToMediumBoat::EntryMoveToMediumBoat()
{
}

EntryMoveToMediumBoat::~EntryMoveToMediumBoat()
{
}

int EntryMoveToMediumBoat::getTargetId()
{
    return 7;
}

bool EntryMoveToMediumBoat::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryMoveToMediumBoat::execute");
}


EntryMoveToMediumSoldier::EntryMoveToMediumSoldier()
{
}

EntryMoveToMediumSoldier::~EntryMoveToMediumSoldier()
{
}

int EntryMoveToMediumSoldier::getTargetId()
{
    return 7;
}

// 0x2cc400 (Ghidra). A step of a move to a position by hand: the soldier's matrix goes the distance
// of this frame at its pose's speed, without physics. In the last half metre it steps on along its
// facing.
bool EntryMoveToMediumSoldier::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPAMoveTo* const move = static_cast<BAPAMoveTo*>(script);
    BotMain* const main = static_cast<BotMain*>(bot);
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    if (!equipment)
    {
        done = true;
        ok = false;
        return true;
    }
    IAIObject* const object = equipment->getAIObject();
    sleepPhysics(object);
    if (move->isFinished(bot))
    {
        done = true;
        ok = true;
        return true;
    }
    done = false;
    infanteryResetControls(bot, main->getPlayerInput(), true, false, false);
    Vec3 position = object->getPosition();
    Vec2 here;
    here.x = 0.0f;
    here.y = 0.0f;
    vec3ToVec2XZ(position, here);
    Vec2 point = here;
    point.x = 0.0f;
    point.y = 0.0f;
    bool stopped = false;
    ok = move->update(point, done, stopped, 0.0f);
    if (!ok)
    {
        done = true;
        return true;
    }
    if (done || stopped)
        return true;
    Vec2 to;
    to.x = point.x - here.x;
    to.y = point.y - here.y;
    const float distance = std::sqrt(to.y * to.y + to.x * to.x);
    IAIObjectMobile* const mobile = object->getMobile();
    const float step = static_cast<float>(
        static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getTimeIncrease());
    float speed = mobile->getMaxSpeed();
    bot->requestSoldierPose(SPStanding, false);
    const int pose = equipment->getSoldierPose();
    speed = poseSpeed(speed, pose);
    Mat4 matrix = object->getTransformation();
    Vec3 forward;
    bool along = true;
    float x = to.x;
    float z = to.y;
    if (step * speed <= distance)
    {
        if (9.9999997e-06f < distance)
        {
            to.x = to.x * (1.0f / distance);
            to.y = (1.0f / distance) * to.y;
        }
        z = step * to.y * speed;
        x = to.x * step * speed;
    }
    else if (distance <= 0.5f)
    {
        along = false;
        forward.x = matrix.m[8];
        forward.y = matrix.m[9];
        forward.z = matrix.m[10];
        x = matrix.m[8] * 0.5f;
        z = 0.5f * matrix.m[10];
    }
    position.x = x + position.x;
    position.z = z + position.z;
    if (along)
        vec2ToVec3XZ(to, forward);
    normalizeOrZero(forward);
    placeSoldier(object, mobile, matrix, position, forward, pose, equipment->getPosition().y, false, bot->getSide());
    return true;
}


EntryMoveToObjectMedium::EntryMoveToObjectMedium()
{
}

EntryMoveToObjectMedium::~EntryMoveToObjectMedium()
{
}

int EntryMoveToObjectMedium::getTargetId()
{
    return 9;
}

bool EntryMoveToObjectMedium::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryMoveToObjectMedium::execute");
}


EntryMoveToObjectMediumBoat::EntryMoveToObjectMediumBoat()
{
}

EntryMoveToObjectMediumBoat::~EntryMoveToObjectMediumBoat()
{
}

int EntryMoveToObjectMediumBoat::getTargetId()
{
    return 9;
}

bool EntryMoveToObjectMediumBoat::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryMoveToObjectMediumBoat::execute");
}


EntryMoveToObjectMediumSoldier::EntryMoveToObjectMediumSoldier()
{
}

EntryMoveToObjectMediumSoldier::~EntryMoveToObjectMediumSoldier()
{
}

int EntryMoveToObjectMediumSoldier::getTargetId()
{
    return 9;
}

// 0x2cefe0 (Ghidra). A step of a move to an object by hand, as EntryMoveToMediumSoldier. At the point
// (within 1 cm) the soldier stays where it is.
bool EntryMoveToObjectMediumSoldier::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPAMoveTo* const move = static_cast<BAPAMoveTo*>(script);
    BotMain* const main = static_cast<BotMain*>(bot);
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    if (!equipment)
    {
        done = true;
        ok = false;
        return true;
    }
    IAIObject* const object = equipment->getAIObject();
    IAIObjectMobile* const mobile = object->getMobile();
    sleepPhysics(object);
    infanteryResetControls(bot, main->getPlayerInput(), true, false, false);
    if (!informationManager->getInformation(move->getObject()) || move->isFinished(bot))
    {
        done = true;
        ok = true;
        return true;
    }
    done = false;
    Vec2 point;
    point.x = 0.0f;
    point.y = 0.0f;
    bool stopped = false;
    ok = move->update(point, done, stopped, 0.0f);
    if (!ok)
    {
        done = true;
        return true;
    }
    if (done || stopped)
    {
        done = false;
        return true;
    }
    const Vec3& here = equipment->getPosition();
    Vec2 to;
    to.x = point.x - here.x;
    to.y = point.y - here.z;
    const float distance = std::sqrt(to.y * to.y + to.x * to.x);
    if (distance <= 0.01f)
    {
        done = false;
        return true;
    }
    Mat4 matrix = object->getTransformation();
    const float step = static_cast<float>(
        static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getTimeIncrease());
    bot->requestSoldierPose(SPStanding, false);
    const int pose = equipment->getSoldierPose();
    const float speed = poseSpeed(mobile->getMaxSpeed(), pose);
    Vec3 position = object->getPosition();
    Vec3 forward;
    forward.x = to.x / distance;
    forward.y = 0.0f;
    forward.z = to.y / distance;
    float x = to.x;
    float z = to.y;
    if (step * speed <= distance)
    {
        z = step * forward.z * speed;
        x = forward.x * step * speed;
    }
    position.x = position.x + x;
    position.z = position.z + z;
    placeSoldier(object, mobile, matrix, position, forward, pose, equipment->getPosition().y, true, bot->getSide());
    return true;
}


EntryInfoWrapper::EntryInfoWrapper()
{
}

EntryInfoWrapper::~EntryInfoWrapper()
{
}

int EntryInfoWrapper::getTargetId()
{
    return 15;
}

bool EntryInfoWrapper::execute(BotScript* script, Bot* bot, bool& ok, bool& done)
{
    BAPStatement* const statement = static_cast<BAPInfoWrapper*>(script)->m_statement;
    return static_cast<BotMain*>(bot)->getPlanInterpreter()->execute(statement, bot, ok, done);
}


EntryHeliMoveTo::EntryHeliMoveTo()
{
}

EntryHeliMoveTo::~EntryHeliMoveTo()
{
}

int EntryHeliMoveTo::getTargetId()
{
    return 7;
}

bool EntryHeliMoveTo::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryHeliMoveTo::execute");
}


EntryHeliMoveToDirection::EntryHeliMoveToDirection()
{
}

EntryHeliMoveToDirection::~EntryHeliMoveToDirection()
{
}

int EntryHeliMoveToDirection::getTargetId()
{
    return 8;
}

bool EntryHeliMoveToDirection::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryHeliMoveToDirection::execute");
}


EntryHeliMoveToObject::EntryHeliMoveToObject()
{
}

EntryHeliMoveToObject::~EntryHeliMoveToObject()
{
}

int EntryHeliMoveToObject::getTargetId()
{
    return 9;
}

bool EntryHeliMoveToObject::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryHeliMoveToObject::execute");
}


EntryHeliAimAt::EntryHeliAimAt()
{
}

EntryHeliAimAt::~EntryHeliAimAt()
{
}

int EntryHeliAimAt::getTargetId()
{
    return 6;
}

bool EntryHeliAimAt::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryHeliAimAt::execute");
}


EntryHeliResetControls::EntryHeliResetControls()
{
}

EntryHeliResetControls::~EntryHeliResetControls()
{
}

int EntryHeliResetControls::getTargetId()
{
    return 14;
}

bool EntryHeliResetControls::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryHeliResetControls::execute");
}


EntryHeliMoveAwayFromObject::EntryHeliMoveAwayFromObject()
{
}

EntryHeliMoveAwayFromObject::~EntryHeliMoveAwayFromObject()
{
}

int EntryHeliMoveAwayFromObject::getTargetId()
{
    return 10;
}

bool EntryHeliMoveAwayFromObject::execute(BotScript *, Bot *, bool&, bool&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::EntryHeliMoveAwayFromObject::execute");
}


} // namespace ai
} // namespace hfe
} // namespace dice
