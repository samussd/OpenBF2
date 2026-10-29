#include "BAPAMergeSoldierInputs.h"

#include "../AIBot/BotMain.h"                    // Bot
#include "../AIBot/SensingAgent.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIPlanInterpreter/MouseControl.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/IBotManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../../../../Game/Objects/ObjectLayoutAccess.h"
#include "../../../../IO/Input/PlayerInput.h"
#include "../../../../Math/Mat4.h"
#include "../../../../Physics/IPhysicsNode.h"
#include <cfloat>
#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

BAPAMergeSoldierInputs::BAPAMergeSoldierInputs(int planId)
    : BAPAction(planId)
    , m_float20(0.0f)
    , m_float24(0.0f)
{
}

BAPAMergeSoldierInputs::~BAPAMergeSoldierInputs()
{
}

namespace {

// 1, -1 or 0.
float sign(float value)
{
    if (value > 0.0f)
        return 1.0f;
    if (value < 0.0f)
        return -1.0f;
    return 0.0f;
}

float clampUnit(float value)
{
    if (value < -1.0f)
        return -1.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}

// The 2D normalise the binary inlines: unit length untouched, shorter than FLT_EPSILON squared zero.
void normaliseXZ(Vec2& v)
{
    const float lengthSquared = v.x * v.x + v.y * v.y;
    if (std::fabs(lengthSquared - 1.0f) < FLT_EPSILON)
        return;
    if (std::fabs(lengthSquared) < 1.4210855e-14f)
    {
        v.x = 0.0f;
        v.y = 0.0f;
        return;
    }
    const float scale = 1.0f / std::sqrt(lengthSquared);
    v.x *= scale;
    v.y *= scale;
}

float axis(const io::PlayerInput* input, io::PlayerInputMap which)
{
    return (input->mappingFlags >> which) & 1 ? input->inputs[which] : 0.0f;
}

} // namespace

// The sensing agent's move direction becomes the soldier's inputs. Walking: projected on the
// soldier's forward and right (turned ahead by its rotation over lookAheadFrames frames), square
// rooted, into throttle and strafe at the throttle's strength; no sprint. Sprinting (or swimming):
// the view turns along the direction and the soldier runs, sprinting while more than 4 m from
// the move target and within 40 degrees of it, and stops when close and more than 5 degrees off.
// Always done.
bool BAPAMergeSoldierInputs::execute(Bot* bot, bool& ok, bool& done)
{
    io::PlayerInput* const input = static_cast<BotMain*>(bot)->getPlayerInput();
    SensingAgent* const agent = bot->getSensingAgent();
    const Vec2 move = agent->getMoveDirection();
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const Mat4& transformation = equipment->getTransformation();
    IPIControlInfo* const control = equipment->getControlInfo();
    float pitchError = 0.0f;
    float yawError = 0.0f;
    const int pose = equipment->getSoldierPose();
    const bool sprinting = axis(input, io::PISprint) != 0.0f;

    if (!sprinting && pose != 4)
    {
        Vec2 forward;
        forward.x = 0.0f;
        forward.y = 0.0f;
        Vec2 right;
        right.x = 0.0f;
        right.y = 0.0f;
        Vec3 row;
        row.x = transformation.m[8];
        row.y = transformation.m[9];
        row.z = transformation.m[10];
        vec3ToVec2XZ(row, forward);
        row.x = transformation.m[0];
        row.y = transformation.m[1];
        row.z = transformation.m[2];
        vec3ToVec2XZ(row, right);
        normaliseXZ(forward);
        normaliseXZ(right);

        IAIObject* const object = equipment->getAIObject();
        if (object)
        {
            world::IPhysicsNode* const node = static_cast<world::IPhysicsNode*>(
                world::object_detail::LayoutAccess::physicsNode(object->getObject()));
            if (node)
            {
                static int lookAheadFrames = 3;
                const float ahead = static_cast<float>(lookAheadFrames) * static_cast<float>(
                    static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getTimeIncrease());
                const float turnRate = 0.017453292f * node->getRotationalSpeed().y;
                if (turnRate != 0.0f)
                {
                    const float angle = turnRate * ahead;
                    const float c = std::cos(angle);
                    const float s = std::sin(angle);
                    const Vec2 f = forward;
                    forward.x = c * f.x + s * f.y;
                    forward.y = -s * f.x + c * f.y;
                    const Vec2 r = right;
                    right.x = s * r.y + c * r.x;
                    right.y = -s * r.x + c * r.y;
                }
            }
        }

        const float strength = std::fabs(axis(input, io::PIThrottle));
        const float along = forward.x * move.x + forward.y * move.y;
        const float throttle = clampUnit(std::sqrt(std::fabs(along)) * sign(along));
        const float across = right.x * move.x + right.y * move.y;
        const float strafe = clampUnit(std::sqrt(std::fabs(across)) * sign(across));
        input->setInput(io::PIThrottle, throttle * strength);
        input->setInput(io::PIYaw, strength * strafe);
        input->setInput(io::PISprint, 0.0f);
    }
    else
    {
        Vec3 direction;
        direction.x = 0.0f;
        direction.y = 0.0f;
        direction.z = 0.0f;
        vec2ToVec3XZ(move, direction);
        mouseControlLookAtDirection(bot, direction,
            *static_cast<const Mat4*>(bot->getPlayer()->getCameraTransformation()),
            pitchError, m_float24, yawError, m_float20, 0.0f, control, input,
            0.0f, 0.0f, 0.0f);

        float runDistance = 16.0f;
        bool occupiedTarget = false;
        float distanceSquared = 0.0f;
        if (sprinting)
        {
            const float x = transformation.m[12];
            const float z = transformation.m[14];
            Vec3 target;
            target.x = 0.0f;
            target.y = 0.0f;
            target.z = 0.0f;
            Information* const info = agent->getMoveTarget() == 0xFFFFFFFFu
                ? 0 : informationManager->getInformation(agent->getMoveTarget());
            if (info)
            {
                target = info->getPosition();
                // a vehicle someone (not a bot) sits in
                occupiedTarget = info->getAIObject()
                    && !static_cast<IBotManager*>(IBotManager::instance)->getBot(info->getAIObject())
                    && info->getUnit() && info->getUnit()->isOccupied();
            }
            else
            {
                target = agent->getMoveTargetPosition();
            }
            const float dx = x - target.x;
            const float dz = z - target.z;
            distanceSquared = dx * dx + 0.0f * 0.0f + dz * dz;
        }

        const bool closeToOccupied =
            agent->getMoveTarget() != 0xFFFFFFFFu && distanceSquared < 9.0f && occupiedTarget;
        if (closeToOccupied)
            runDistance = 1.0f;

        if (!(runDistance > distanceSquared) || closeToOccupied
            || !(std::fabs(yawError) > 0.08726646f))
        {
            if ((std::fabs(yawError) < 0.69813168f || closeToOccupied)
                && distanceSquared > runDistance)
            {
                if (sprinting)
                    input->setInput(io::PISprint, 1.0f);
                input->setInput(io::PIThrottle, 1.0f);
            }
            else
            {
                input->setInput(io::PIThrottle, 1.0f);
                input->setInput(io::PISprint, 0.0f);
            }
        }
        else
        {
            if ((input->mappingFlags >> io::PIThrottle) & 1)
                input->inputs[io::PIThrottle] = 0.0f;
            input->setInput(io::PISprint, 0.0f);
        }
    }

    m_float24 = pitchError;
    m_float20 = yawError;
    ok = true;
    done = true;
    return true;
}

BAPStatementType BAPAMergeSoldierInputs::getId() const
{
    return BAPSTMergeSoldierInputs;
}

bool BAPAMergeSoldierInputs::hasDefault()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
