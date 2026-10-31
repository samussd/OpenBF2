#include "InfanteryControl.h"

#include "../AIBot/BotMain.h"
#include "../AIBot/BotNavigation.h"
#include "../AIBot/SquadMember.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIArmament.h"
#include "../AIKnowledge/IPIMobile.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIKnowledge/WeaponInfo.h"
#include "../AIKnowledge/WeaponInfoManager.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/IAIPathfinding.h"
#include "../Main/IBotManager.h"
#include "../../../../IO/Input/PlayerInput.h"
#include "../../../../Math/Mat4.h"

#include <cmath>
#include <vector>

namespace dice {
namespace hfe {
namespace ai {

// The look group reads the equipment without a null guard: element 9 of its transformation (the
// forward vector's y) is negated into the vertical look.
bool infanteryResetControls(Bot* bot, io::PlayerInput* input, bool movement, bool look, bool actions)
{
    if (movement)
    {
        input->setInput(io::PIAction, 0.0f);
        input->setInput(io::PISprint, 0.0f);
        input->setInput(io::PIYaw, 0.0f);
        input->setInput(io::PIRoll, 0.0f);
        input->setInput(io::PIPitch, 0.0f);
        input->setInput(io::PIThrottle, 0.0f);
        input->setInput(io::PIMouseLookX, 0.0f);
        input->setInput(io::PIMouseLookY, 0.0f);
    }
    if (look)
    {
        // [PLATFORM] ELF captures the manager before the equipment callback.
        InformationManager* const manager = informationManager;
        Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
        const Mat4& transformation = equipment->getTransformation();
        input->setInput(io::PIMouseLookX, 0.0f);
        input->setInput(io::PIMouseLookY, -transformation.m[9]);
    }
    if (actions)
    {
        input->setInput(io::PIFire, 0.0f);
        input->setInput(io::PIAltFire, 0.0f);
        input->setInput(io::PIWeaponSelect1, 0.0f);
        input->setInput(io::PIWeaponSelect2, 0.0f);
        input->setInput(io::PIWeaponSelect3, 0.0f);
        input->setInput(io::PIWeaponSelect4, 0.0f);
        input->setInput(io::PIWeaponSelect5, 0.0f);
        input->setInput(io::PIWeaponSelect6, 0.0f);
        input->setInput(io::PIWeaponSelect7, 0.0f);
        input->setInput(io::PIWeaponSelect8, 0.0f);
        input->setInput(io::PIPositionSelect1, 0.0f);
        input->setInput(io::PIPositionSelect2, 0.0f);
        input->setInput(io::PIPositionSelect3, 0.0f);
        input->setInput(io::PIPositionSelect4, 0.0f);
        input->setInput(io::PIPositionSelect5, 0.0f);
        input->setInput(io::PIPositionSelect6, 0.0f);
        input->setInput(io::PIPositionSelect7, 0.0f);
        input->setInput(io::PIPositionSelect8, 0.0f);
        input->setInput(io::PIUse, 0.0f);
        input->setInput(io::PIReload, 0.0f);
        input->setInput(io::PIFlareFire, 0.0f);
    }
    return true;
}

// 0x2db200 (Ghidra). Lets go of the zoom input; a zoomed weapon gets it pressed once to leave the
// zoom. No null guard on the equipment, as shipped.
void removeZoom(Bot* bot)
{
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    IPIArmament* const armament = equipment->getArmament();
    if (!armament)
        return;
    armament->getWeapon(armament->getActiveWeapon());
    WeaponInfo* const weapon =
        WeaponInfoManager::m_instance->getInformation(armament->getWeapon(armament->getActiveWeapon()));
    if (!weapon || weapon->getWeaponZoom() == io::PINone)
        return;
    io::PlayerInput* const input = static_cast<BotMain*>(bot)->getPlayerInput();
    const io::PlayerInputMap zoom = static_cast<io::PlayerInputMap>(weapon->getWeaponZoom());
    if (!weapon->isZoomed())
        input->setInput(zoom, 0.0f);
    else if (zoom < io::PINone && input->getInput(zoom) != 0.0f)
        input->setInput(zoom, 0.0f);
    else
        input->setInput(zoom, 1.0f);
}

// 0x2db550 (Ghidra). Sprinting is full throttle (the sign of maxSpeed); walking is the throttle that
// gives the squad member's speed. A bot simulating artillery only crouches, and makes no progress.
void infanteryControlThrottle(Bot* bot, io::PlayerInput* input, IPIControlInfo*, float maxSpeed, bool sprint)
{
    BotMain* const main = static_cast<BotMain*>(bot);
    if (main->getBotIsSimulatingArtillery())
    {
        main->getBotNavigation()->clearProgress();
        bot->requestSoldierPose(SPCrouching, true);
        return;
    }
    const float speed = bot->getSquadMember()->getMaxSpeed();
    if (sprint)
    {
        float throttle = 1.0f;
        if (maxSpeed <= 0.0f)
            throttle = maxSpeed < 0.0f ? -1.0f : 0.0f;
        input->setInput(io::PIThrottle, throttle);
        input->setInput(io::PISprint, 1.0f);
    }
    else
    {
        input->setInput(io::PIThrottle, speed / maxSpeed);
        input->setInput(io::PISprint, 0.0f);
    }
}

namespace {

// How far a trace got from the obstacle; a hit left at the origin counts as no room.
float roomFrom(const Vec3& from, const Vec3& hit)
{
    if (std::fabs(hit.x) <= 1.1920929e-07f && std::fabs(hit.y) <= 1.1920929e-07f
        && std::fabs(hit.z) <= 1.1920929e-07f)
        return 0.0f;
    const float x = hit.x - from.x;
    const float y = hit.y - from.y;
    const float z = hit.z - from.z;
    return std::sqrt(x * x + y * y + z * z);
}

} // namespace

// 0x2dbc90 (Ghidra).
bool needsUpdate(const Vec2& point, unsigned int obstacle, float radius)
{
    Information* const object = informationManager->getInformation(obstacle);
    if (!object)
        return false;
    Vec2 at;
    at.x = 0.0f;
    at.y = 0.0f;
    vec3ToVec2XZ(object->getPosition(), at);
    const float distanceSqr = (point.y - at.y) * (point.y - at.y) + (point.x - at.x) * (point.x - at.x);
    return distanceSqr < radius * radius || (radius + 1.5f) * (radius + 1.5f) < distanceSqr;
}

// 0x2dbd70 (Ghidra). Picks the nearest obstacle on the way to `ahead`: a vehicle-like object (a
// mobile or flag 0x2000000, not a soldier) within 2 m in height and its radius + 1 m, in front
// (cosine at least 0.1) and not already passed to the side. A flagged object, or one below the
// vehicle bail value, counts as weak: unless it is the bot's change or repair target its radius is
// the smallest radius + 4, at most 10 and never below the smallest, times 1.3. Traces of 15 m to
// each side of the obstacle give the room there. obstacleFlag: weak.
// True: the static obstacle stands off the mesh. The coordinate is not used; no null guard on the
// equipment, as shipped.
bool updateInfantryMoveToPosWithObstacles(Bot* bot, const Vec2& ahead, const Vec3&, unsigned int object,
                                          Vec2& avoidPoint, unsigned int& obstacle, float& passDistance,
                                          Vec2& obstaclePosition, float& obstacleRadius, bool& obstacleFlag,
                                          Information* information, bool flag)
{
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    const std::vector<unsigned int>& obstacles = bot->getBotNavigation()->getObstacles();
    float nearest = 99999.0f;
    Vec2 nearestAt;
    nearestAt.x = 0.0f;
    nearestAt.y = 0.0f;
    const Vec3& at = equipment->getPosition();
    Vec2 position = nearestAt;
    vec3ToVec2XZ(at, position);
    Vec2 forward;
    forward.x = ahead.x - position.x;
    forward.y = ahead.y - position.y;
    const float length = std::sqrt(forward.y * forward.y + forward.x * forward.x);
    if (length < 9.9999997e-05f)
        return false;
    const float inverse = 1.0f / length;
    forward.x = forward.x * inverse;
    forward.y = inverse * forward.y;
    Vec2 normal = nearestAt;
    getNormal(forward, normal);
    IPIMobile* nearestMobile = 0;
    Information* info = informationManager->getInformation(object);
    Vec3 nearestPosition;
    nearestPosition.x = 0.0f;
    nearestPosition.y = 0.0f;
    nearestPosition.z = 0.0f;
    if (info && !info->isPrimary())
        info->getPrimaryObject(bot->getSide());
    passDistance = 0.0f;
    unsigned int change = bot->getChangeTarget();
    info = informationManager->getInformation(change);
    if (info && !info->isPrimary())
        change = info->getPrimaryObject(bot->getSide());
    unsigned int repair = bot->getRepairTarget();
    info = informationManager->getInformation(repair);
    if (info && !info->isPrimary())
        repair = info->getPrimaryObject(bot->getSide());
    for (unsigned int i = 0; i < obstacles.size(); ++i)
    {
        const unsigned int handle = obstacles[i];
        Information* const candidate = informationManager->getInformation(handle);
        if (equipment == candidate || (candidate == information && !flag) || !candidate)
            continue;
        IPIPhysical* const physical = candidate->getPhysical();
        if (!physical || (candidate->getObjectTypeFlags() & 0x200000) != 0)
            continue;
        IPIMobile* const mobile = candidate->getMobile();
        if (!mobile && (candidate->getObjectTypeFlags() & 0x2000000) == 0)
            continue;
        const bool ownTarget = handle == repair || handle == change;
        const float health = physical->getHealth();
        const bool weak = (candidate->getObjectTypeFlags() & 0x2000000) != 0
            || health < IBotManager::instance->getVehicleBailValue();
        float radius;
        if (!weak || ownTarget)
            radius = physical->getSmallestRadius();
        else
        {
            const float smallest = physical->getSmallestRadius();
            float wide = physical->getSmallestRadius() + 4.0f;
            if (10.0f < wide)
                wide = 10.0f;
            if (wide <= smallest)
                wide = smallest;
            radius = wide * 1.3f;
        }
        if (std::fabs(candidate->getPosition().y - at.y) > 2.0f)
            continue;
        Vec2 candidateAt;
        candidateAt.x = 0.0f;
        candidateAt.y = 0.0f;
        vec3ToVec2XZ(candidate->getPosition(), candidateAt);
        const float toX = candidateAt.x - position.x;
        const float toY = candidateAt.y - position.y;
        const float distanceSqr = toX * toX + toY * toY;
        const float along = toX * forward.x + toY * forward.y;
        if (!(distanceSqr <= (radius + 1.0f) * (radius + 1.0f)))
            continue;
        if (!(0.1f <= along / std::sqrt(distanceSqr)))
            continue;
        if (radius + 1.0f <= std::fabs(toX * normal.x + toY * normal.y) || nearest <= along)
            continue;
        nearestAt = candidateAt;
        obstacleRadius = radius;
        nearestPosition = candidate->getPosition();
        obstacle = handle;
        obstaclePosition = candidateAt;
        obstacleFlag = weak;
        nearestMobile = mobile;
        nearest = along;
    }
    if (!(nearest < 99999.0f))
        return false;

    IAIPathfinding* const pathfinding = IAIPathfinding::instance;
    const int vehicle = equipment->getMobile()->getVehicleNumber();
    bot->getBotNavigation();
    const int type = equipment->getUnit()->getEquipmentType();
    Vec3 hit0;
    hit0.x = 0.0f;
    hit0.y = 0.0f;
    hit0.z = 0.0f;
    Vec3 hit1 = hit0;
    Vec3 side = hit0;
    vec2ToVec3XZ(normal, side);
    Vec3 reach;
    reach.x = side.x * 15.0f;
    reach.y = side.y * 15.0f;
    reach.z = 15.0f * side.z;
    unsigned int last = 0;
    if (!nearestMobile)
    {
        if (!pathfinding->isValidPosition(vehicle, type, nearestPosition, true))
            return true;
        const Vec3 to0 = { reach.x + nearestPosition.x, reach.y + nearestPosition.y, reach.z + nearestPosition.z };
        pathfinding->trace(vehicle, nearestPosition, to0, hit0, last, type, -1, 0.0f, 0, 0);
        const Vec3 to1 = { nearestPosition.x - reach.x, nearestPosition.y - reach.y, nearestPosition.z - reach.z };
        pathfinding->trace(vehicle, nearestPosition, to1, hit1, last, type, -1, 0.0f, 0, 0);
    }
    else
    {
        const IPIMobile::ValidPos& place = nearestMobile->getLastValidPosition(vehicle);
        const Vec3 to0 = { reach.x + place.position.x, reach.y + place.position.y, reach.z + place.position.z };
        pathfinding->trace(place.polygon, place.quadTree, vehicle, place.position, to0, hit0, last, type, -1, 0.0f,
                           0, 0);
        const Vec3 to1 = { place.position.x - reach.x, place.position.y - reach.y, place.position.z - reach.z };
        pathfinding->trace(place.polygon, place.quadTree, vehicle, place.position, to1, hit1, last, type, -1, 0.0f,
                           0, 0);
    }
    const float room0 = roomFrom(nearestPosition, hit0);
    const float room1 = roomFrom(nearestPosition, hit1);
    avoidPoint = nearestAt;
    const float radius = obstacleRadius;
    // The bot's own side of the obstacle wins if it has the room or the most of it.
    bool otherSide;
    if ((nearestAt.y - position.y) * normal.y + (nearestAt.x - position.x) * normal.x <= 0.0f)
        otherSide = !(radius + 1.0f < room0 || room1 < room0);
    else
        otherSide = radius + 1.0f < room1 || room0 < room1;
    const float room = otherSide ? room1 : room0;
    const float pass = radius > room ? room : radius;
    const float offset = otherSide ? -pass : pass;
    avoidPoint.x = normal.x * offset + avoidPoint.x;
    avoidPoint.y = offset * normal.y + avoidPoint.y;
    passDistance = pass;
    return false;
}

} // namespace ai
} // namespace hfe
} // namespace dice
