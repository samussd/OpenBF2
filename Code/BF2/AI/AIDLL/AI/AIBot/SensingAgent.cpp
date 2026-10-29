// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SensingAgent operations.

#include "SensingAgent.h"

#include "../AIPlanInterpreter/PlanInterpreter.h"
#include "../AIBotActionPlan/BAPStatement.h"

#include "BotMain.h"                       // executePlan: the plan interpreter
#include "BotSenses.h"
#include "SensedObject.h"
#include "../AIKnowledge/FireObject.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIControlInfo.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIPlanInterpreter/BAPContext.h"
#include "SensingAgentPatterns.h"           // ISensingAgentPattern::reset - the shared patterns
#include "../Main/AIRandom.h"               // the per-life reaction time
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"   // IBFTimer::getGameTime
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../../../../Core/Debug.h"
#include "../../../../IO/Input/PlayerInput.h"
#include "../../../../Math/Mat4.h"
#include <cfloat>
#include <cmath>
#include <map>
#include "../AIUtils/checkPointer.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

// How long an override request blocks the sensing plan before it lapses.
// SensingAgent::update()::waitTime in the binary, a constant-initialised function static.
const double kOverrideWaitTime = 2.0;

// The 2D normalise the binary inlines: a unit vector is left alone, a vector shorter than
// FLT_EPSILON squared becomes zero (and the result is false).
bool normaliseXZ(Vec2& v)
{
    const float lengthSquared = v.x * v.x + v.y * v.y;
    if (std::fabs(lengthSquared - 1.0f) < FLT_EPSILON)
        return true;
    if (std::fabs(lengthSquared) < 1.4210855e-14f)
    {
        v.x = 0.0f;
        v.y = 0.0f;
        return false;
    }
    const float scale = 1.0f / std::sqrt(lengthSquared);
    v.x *= scale;
    v.y *= scale;
    return true;
}

// The 3D one; this one zeroes below FLT_EPSILON itself.
void normalise(Vec3& v)
{
    const float lengthSquared = v.x * v.x + v.y * v.y + v.z * v.z;
    if (std::fabs(lengthSquared - 1.0f) < FLT_EPSILON)
        return;
    if (std::fabs(lengthSquared) < FLT_EPSILON)
    {
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        return;
    }
    const float scale = 1.0f / std::sqrt(lengthSquared);
    v.x *= scale;
    v.y *= scale;
    v.z *= scale;
}

// Clamped into acos's domain; NaN passes through.
float angleFromCosine(float cosine)
{
    if (cosine < -1.0f)
        cosine = -1.0f;
    else if (cosine > 1.0f)
        cosine = 1.0f;
    return std::acos(cosine);
}

// A camera's forward row turned about Y by the control's depth-of-field offset (radians).
void applyDofRotation(Vec3& forward, float dofOffset)
{
    if (!(dofOffset > 0.0f))
        return;
    Mat4 rotation = Mat4::getIdentity();
    setRotateYDeg(rotation, dofOffset * 57.29578f);
    const float* const r = rotation.m;
    const Vec3 f = forward;
    forward.x = f.x * r[0] + f.y * r[4] + f.z * r[8];
    forward.y = f.x * r[1] + f.y * r[5] + f.z * r[9];
    forward.z = f.x * r[2] + f.y * r[6] + f.z * r[10];
}

Vec3 forwardRow(const Mat4& m)
{
    Vec3 forward;
    forward.x = m.m[8];
    forward.y = m.m[9];
    forward.z = m.m[10];
    return forward;
}

// [lo, hi), drawn in the binary's extended precision rather than through nextFloat.
float randomInRange(float lo, float hi)
{
    const double unit = static_cast<double>(AIRandom::m_instance->next()) * 2.3283064365386963e-10;
    return static_cast<float>(lo + unit * (hi - lo));
}

float playerInput(const io::PlayerInput* input, io::PlayerInputMap which)
{
    return (input->mappingFlags >> which) & 1 ? input->inputs[which] : 0.0f;
}

double gameTime()
{
    return static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
}

} // namespace

// Creates sensing state and an eight-sector attention vector for the bot.
SensingAgent::SensingAgent(BotMain* owner, unsigned char /*behaviourCount*/)
    : m_flag134(false)
    , m_plan(0)
{
    m_currentQuadPosition.x = 0.0f;        // +32, +36, +40
    m_currentQuadPosition.y = 0.0f;
    m_currentQuadPosition.z = 0.0f;
    m_planDirection.x = 0.0f;              // +64, +68, +72
    m_planDirection.y = 0.0f;
    m_planDirection.z = 0.0f;
    m_lookDirection.x = 0.0f;              // +80, +84
    m_lookDirection.y = 0.0f;
    for (int i = 0; i < 3; ++i)
        m_zero88[i] = 0;                   // +88..+96

    // the mechanism this agent steers. It takes the bot rather than the agent, so `BotSenses`
    // answers to the bot directly and this object is a policy layer over it.
    m_senses = checkPointer(new BotSenses(owner));
    m_bot    = owner;

    m_bapContext = checkPointer(new BAPContext());

    // eight quadrants, zeroed.
    m_quadrantWeights.resize(8, 0.0f);

    // A sensing agent is only half built until `reset` has run - the same constructor/reset
    // split `BotMain` uses.
    reset();
}

SensingAgent::~SensingAgent()
{
    delete m_senses;
    delete m_bapContext;
}

// Deletes the owned plan and marks planning state for refresh.
void SensingAgent::deletePlan()
{
    if (m_plan)
    {
        delete m_plan;
        m_plan = 0;
    }
    m_planInvalid = true;
}

// Clears the current plan's target information.
void SensingAgent::clearPlanTargetInformation()
{
    m_planTarget = 0;
    m_planDirectionWeight = 0.0f;
}

// Resets sensing state and draws a new reaction delay in [0.5, 1.0).
void SensingAgent::reset()
{
    m_forceUpdate = false;
    m_enemyFireNoticed = false;
    m_lastUpdateTime = 0.0;
    m_overrideRequested = false;
    m_overrideDisallowed = false;
    m_planInvalid = true;
    m_lookingForward = false;

    // the shared patterns, re-indexed.
    IAISettings* const settings =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    const std::map<int, ISensingAgentPattern*>& patterns = settings->getSensingAgentPatterns();

    int index = 0;
    for (std::map<int, ISensingAgentPattern*>::const_iterator it = patterns.begin();
         it != patterns.end(); ++it)
    {
        it->second->reset(index++);
    }

    deletePlan();

    m_currentQuad = -1;
    m_currentQuadWeight = 0.0f;
    m_currentQuadSeen = false;
    m_lookingIntoQuadrant = true;
    m_lookDirection.x = 0.0f;              // +80
    m_lookDirection.y = 0.0f;              // +84

    // through the buffer; the size is not touched.
    for (unsigned q = 0; q <= 7; ++q)
        m_quadrantWeights[q] = 0.0f;

    clearPlanTargetInformation();

    // game time, not wall-clock.
    m_lastUpdateTime = gameTime();
    m_overrideTime = 0.0;

    // Chooses a reaction time in [0.5, 1.0), preserving the arithmetic order.
    m_reactionTime = 0.5f + AIRandom::m_instance->nextFloat() * (1.0f - 0.5f);

    m_senses->reset();
}

// Ignores the override while m_overrideDisallowed is set. Otherwise stores the requested value
// and the current game time. reset clears the refusal flag.
void SensingAgent::setOverride(bool value)
{
    if (!m_overrideDisallowed)
    {
        m_overrideRequested = value;
        m_overrideTime = gameTime();
    }
}

// The tick. Without an override request: once the reaction time has run out (or a replan is
// forced, or the plan is gone) the urgencies age, the focus objects are weighed, a quadrant is
// chosen and the plan is rebuilt. A prone or swimming soldier, or one not allowed to fire,
// just looks along m_lookDirection.
void SensingAgent::update()
{
    Information* const soldier = informationManager->getInformation(m_bot->getCurrentEquipment());
    if (!soldier || m_bot->getManDown())
        return;

    IAISettings* const settings =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    ISensingAgentPattern* const pattern =
        settings->getSensingAgentPattern(soldier->getUnit()->getEquipmentType());
    if (!pattern)
    {
        deletePlan();
        return;
    }

    if (m_overrideRequested)
        deletePlan();

    const double time = gameTime();
    if (m_overrideRequested)
    {
        if (time - m_overrideTime > kOverrideWaitTime)
            m_overrideRequested = false;
        return;
    }

    if (m_lastUpdateTime > time)
        m_lastUpdateTime = time;
    const double elapsed = time - m_lastUpdateTime;

    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    bool behind = false;
    if ((equipment->getObjectTypeFlags() & 0x200000u) && m_lookingForward
        && !isForward(m_currentQuad, m_lookDirection))
    {
        behind = true;
    }

    if (!(m_reactionTime > elapsed) || m_forceUpdate || m_planInvalid)
    {
        if (behind)
            m_lookingForward = false;
    }
    else
    {
        if (!behind)
            return;
        m_lookingForward = false;
    }

    const SoldierPose pose = soldier->getSoldierPose();
    const bool proneOrSwimming = (pose == 3 && !m_forceUpdate) || pose == 4;
    if (!m_bot->isFireAllowed() || proneOrSwimming)
    {
        Vec3 direction;
        direction.x = m_lookDirection.x;
        direction.y = equipment->getTransformation().m[9];
        direction.z = m_lookDirection.y;
        createDirPlan(direction, pattern);
        return;
    }

    Vec2 urgencyDirection;
    urgencyDirection.x = 0.0f;
    urgencyDirection.y = 0.0f;
    if (!m_forceUpdate)
        updateQuadTimeUrgencies(urgencyDirection, behind);

    QuadrantInformation quadrant;
    quadrant.position.x = 0.0f;
    quadrant.position.y = 0.0f;
    quadrant.position.z = 0.0f;
    Vec2 away;
    away.x = -urgencyDirection.x;
    away.y = -urgencyDirection.y;
    calcTargetQuadFromObjects(&quadrant, away);
    calculateCurrentPeriod(&quadrant, equipment, urgencyDirection);

    if (m_forceUpdate)
        deletePlan();
    else
        m_enemyFireNoticed = false;
    m_forceUpdate = false;
    m_lastUpdateTime = time;

    checkForPlanInvalidation(&quadrant, behind);
    if (m_planInvalid)
        createPlan(pattern, &quadrant);
    clearInformation();
}

void SensingAgent::executePlan()
{
    bool ok = false;
    bool done = false;
    m_bot->getPlanInterpreter()->execute(m_plan, m_bot, ok, done);
    m_planInvalid = done || !ok;
    if (m_planInvalid)
        clearPlanTargetInformation();
}

// Also drops a pending override request.
void SensingAgent::disallowOverride(bool disallow)
{
    m_overrideRequested = false;
    m_overrideDisallowed = disallow;
}

void SensingAgent::insertPlanDirectionInformation(float weight, const Vec3& direction, bool force)
{
    m_planDirection = direction;
    m_planTarget = m_bot->getCurrentEquipment();
    m_planDirectionWeight = weight;
    m_forceUpdate = force;
}

void SensingAgent::getQuadPosition(int quad, Vec2& position) const
{
    const float s = 0.70710677f;
    switch (quad)
    {
    case 0: position.x = 0.0f; position.y = 1.0f;  break;
    case 1: position.x = 1.0f; position.y = 0.0f;  break;
    case 2: position.x = 0.0f; position.y = -1.0f; break;
    case 3: position.x = -1.0f; position.y = 0.0f; break;
    case 4: position.x = s;    position.y = s;     break;
    case 5: position.x = s;    position.y = -s;    break;
    case 6: position.x = -s;   position.y = -s;    break;
    case 7: position.x = -s;   position.y = s;     break;
    default:
        Debug(DebugTypeError, "AI/AIDLL/AI/AIBot/SensingAgent.cpp", 770, "")
            << "Bad quadrant value " << quad;
        break;
    }
}

// Within twenty degrees of the quadrant's direction.
bool SensingAgent::isForward(int quad, const Vec2& direction) const
{
    if (direction.x * direction.x + direction.y * direction.y < 0.01f || quad < 0)
        return false;
    Vec2 position;
    position.x = 0.0f;
    position.y = 0.0f;
    getQuadPosition(quad, position);
    return angleFromCosine(direction.x * position.x + direction.y * position.y) < 0.34906584f;
}

// Within ninety degrees of the quadrant's direction.
bool SensingAgent::isBehind(int quad, const Vec2& direction) const
{
    if (direction.x * direction.x + direction.y * direction.y < 0.01f || quad < 0)
        return false;
    Vec2 position;
    position.x = 0.0f;
    position.y = 0.0f;
    getQuadPosition(quad, position);
    return angleFromCosine(direction.x * position.x + direction.y * position.y) < 1.5707964f;
}

// For the plan target the target's world position goes to withinCamView as it is, x and z.
bool SensingAgent::targetFeasible(int quad) const
{
    Vec3 target;
    target.x = 0.0f;
    target.y = 0.0f;
    target.z = 0.0f;
    if (quad >= 0)
    {
        Vec2 position;
        position.x = 0.0f;
        position.y = 0.0f;
        getQuadPosition(quad, position);
        target.x = position.x;
        target.y = 0.0f;
        target.z = position.y;
    }
    else
    {
        Information* const info = informationManager->getInformation(m_planTarget);
        if (!info)
            return false;
        target = info->getPosition();
    }
    Vec2 direction;
    direction.x = target.x;
    direction.y = target.z;
    return withinCamView(direction);
}

// The direction's angle from the vehicle's forward (turned by the camera's depth-of-field
// offset) against the camera's yaw limits, max on one side and min on the other. No limits
// means any direction.
bool SensingAgent::withinCamView(const Vec2& direction) const
{
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    IPIControlInfo* const control = equipment->getControlInfo();
    const float dofOffset = control->getCameraRelativeDofRotationOffset();

    Vec3 forward = forwardRow(m_bot->getPlayer()->getCurrentVehicle()->getTransformation());
    applyDofRotation(forward, dofOffset);

    Vec2 look;
    look.x = forward.x;
    look.y = forward.z;
    normaliseXZ(look);

    const float angle = angleFromCosine(look.x * direction.x + look.y * direction.y);
    const float maxRotation = control->getCameraRelativeMaxRotation().x;
    const float minRotation = control->getCameraRelativeMinRotation().x;
    if (maxRotation == 0.0f && minRotation == 0.0f)
        return true;
    if (look.y * direction.x - look.x * direction.y > 0.0f)
        return !(angle > maxRotation);
    return !(minRotation > -angle);
}

// cos 10 degrees.
bool SensingAgent::quadInFocus(int quad) const
{
    if (quad < 0)
        return false;
    const Mat4& camera = *static_cast<const Mat4*>(m_bot->getPlayer()->getCameraTransformation());
    Vec2 look;
    look.x = camera.m[8];
    look.y = camera.m[10];
    normaliseXZ(look);
    Vec2 position;
    position.x = 0.0f;
    position.y = 0.0f;
    getQuadPosition(quad, position);
    return look.x * position.x + look.y * position.y > 0.9848f;
}

// [UNRESOLVED] caller: nothing recovered so far calls it.
void SensingAgent::setQuadSeen()
{
    const Mat4& camera = *static_cast<const Mat4*>(m_bot->getPlayer()->getCameraTransformation());
    Vec3 ahead;
    ahead.x = camera.m[8] + camera.m[12];
    ahead.y = camera.m[9] + camera.m[13];
    ahead.z = camera.m[10] + camera.m[14];
    int quads[2];
    if (!computeQuadrantsFromPosition(ahead, quads))
        return;
    for (int i = 0; i < 2; ++i)
    {
        if (quadInFocus(quads[i]))
        {
            m_quadrantWeights[quads[i]] = 0.0f;
            if (m_plan && m_lookingIntoQuadrant && quads[i] == m_currentQuad)
                m_currentQuadSeen = true;
        }
    }
}

// The comparisons are written so that NaN takes the ">= 0" side, as the binary's do.
bool SensingAgent::computeQuadrantsFromPosition(const Vec3& position, int* quads) const
{
    const float s = 0.70710677f;
    const Mat4& camera = *static_cast<const Mat4*>(m_bot->getPlayer()->getCameraTransformation());
    Vec2 v;
    v.x = position.x - camera.m[12];
    v.y = position.z - camera.m[14];
    if (!normaliseXZ(v))
        return false;

    const float northEast = s * v.x + s * v.y;
    const float southEast = s * v.x + -s * v.y;
    if (!(northEast < 0.0f))
        quads[0] = !(southEast < 0.0f) ? 1 : 0;
    else
        quads[0] = southEast < 0.0f ? 3 : 2;

    const float north = 0.0f * v.x + 1.0f * v.y;
    const float east = 1.0f * v.x + 0.0f * v.y;
    if (!(north < 0.0f))
        quads[1] = east < 0.0f ? 7 : 4;
    else
        quads[1] = east < 0.0f ? 6 : 5;
    return true;
}

// The old plan goes back to the pattern rather than being deleted.
void SensingAgent::createDirPlan(const Vec3& direction, ISensingAgentPattern* pattern)
{
    if (m_plan)
        m_plan->m_owner = pattern;
    m_plan = pattern->createPlan(this, m_bot, direction);
    if (m_plan)
        m_plan->m_owner = this;
}

void SensingAgent::createNullPlan()
{
    IAISettings* const settings =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    ISensingAgentPattern* const pattern = settings->getSensingAgentNullPattern();
    if (m_plan)
        m_plan->m_owner = pattern;
    m_plan = pattern->createPlan(this, m_bot, 0u);
    if (m_plan)
        m_plan->m_owner = this;
}

// Every quadrant ages: by 0.0125 when it is not ahead of the direction, by 0.05 when it is
// (0.25 while the bot has turned its back on its quadrant). In a vehicle with a mobile primary
// object, the quadrants behind are cleared instead. The direction is minus the vehicle's
// forward, or minus m_lookDirection on foot.
void SensingAgent::updateQuadTimeUrgencies(Vec2& direction, bool behind)
{
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    if (!(equipment->getObjectTypeFlags() & 0x200000u))
    {
        Information* const primary = informationManager->getInformation(
            equipment->getPrimaryObject(m_bot->getSide()));
        const Mat4& transformation = primary->getTransformation();
        direction.x = -transformation.m[8];
        direction.y = -transformation.m[10];
    }
    else
    {
        if (m_lookDirection.x * m_lookDirection.x + m_lookDirection.y * m_lookDirection.y < 0.01f)
            return;
        direction.x = -m_lookDirection.x;
        direction.y = -m_lookDirection.y;
    }

    bool clearBehind = false;
    if (!(equipment->getObjectTypeFlags() & 0x200000u) && equipment->hasMobile()
        && equipment->isPrimary())
    {
        clearBehind = true;
    }

    for (int q = 0; q <= 7; ++q)
    {
        if (clearBehind && isBehind(q, direction))
        {
            m_quadrantWeights[q] = 0.0f;
            continue;
        }
        Vec2 ahead;
        ahead.x = -direction.x;
        ahead.y = -direction.y;
        if (!isForward(q, ahead))
            m_quadrantWeights[q] += 0.0125f;
        else if (behind)
            m_quadrantWeights[q] += 0.25f;
        else
            m_quadrantWeights[q] += 0.05f;
    }
}

void SensingAgent::calcTargetQuadFromObjects(QuadrantInformation* quadrant, const Vec2&)
{
    evaluateSensedObjects();
    evaluateFireObjects();

    std::pair<Vec3, unsigned int> empty;
    empty.first.x = 0.0f;
    empty.first.y = 0.0f;
    empty.first.z = 0.0f;
    empty.second = 0;
    QuadrantWeights weights(8, std::make_pair(0.0f, empty));
    getQuadrantWeights(weights);
    computeTargetQuadrant(weights, quadrant);
}

// A sensed object weighs by how it was sensed, times its security; a physical object without
// health left is skipped.
void SensingAgent::evaluateSensedObjects()
{
    m_sensedFocus.resize(0);

    const std::map<unsigned int, SensedObject*>& sensed = m_senses->getSensedObjects();
    for (std::map<unsigned int, SensedObject*>::const_iterator it = sensed.begin();
         it != sensed.end(); ++it)
    {
        const SensedObject* const object = it->second;
        Information* const info = informationManager->getInformation(object->m_handle);
        if (!info)
            continue;
        if (info->getPhysical() && info->getPhysical()->getHealth() <= 0.0f)
            continue;

        const unsigned int flags = object->m_flags;
        float weight = 0.0f;
        if (flags & SensedObject::kSeen)
        {
            if (flags & SensedObject::kFlag8)
            {
                if ((flags & SensedObject::kFlag20) || (flags & SensedObject::kFlag1))
                    weight += 0.25f;
            }
            else
            {
                weight += 0.5f;
            }
        }
        else
        {
            if (flags & SensedObject::kFlag1)
                weight += 0.3f;
            if (flags & SensedObject::kFlag20)
                weight += 0.25f;
        }

        FocusInformation focus;
        focus.weight = weight * info->getSecurity();
        focus.position = info->getPosition();
        m_sensedFocus.push_back(focus);
    }
}

// Friendly fire focuses on where the shot went at 0.5. Enemy fire forces a replan the first
// time, and weighs 0.7 for five seconds, 0.5 after, at the firer.
void SensingAgent::evaluateFireObjects()
{
    m_fireFocus.resize(0);

    const std::list<std::pair<FireObject*, unsigned int> >& fire = m_senses->getFireObjects();
    const double time = gameTime();
    for (std::list<std::pair<FireObject*, unsigned int> >::const_iterator it = fire.begin();
         it != fire.end(); ++it)
    {
        Information* const info = informationManager->getInformation(it->second);
        if (!info)
            continue;
        const FireObject* const shot = it->first;

        FocusInformation focus;
        if (info->getSide() == m_bot->getSide())
        {
            focus.weight = 0.5f;
            focus.position = shot->m_to;
        }
        else
        {
            if (!m_enemyFireNoticed)
            {
                m_forceUpdate = true;
                m_enemyFireNoticed = true;
            }
            focus.weight = 5.0f > time - shot->m_time ? 0.7f : 0.5f;
            focus.position = info->getPosition();
        }
        focus.weight = info->getSecurity() * focus.weight;
        m_fireFocus.push_back(focus);
    }
}

// Each focus adds its weight and position to the two quadrants it lies in; then every quadrant
// adds its time urgency and the positions are averaged.
void SensingAgent::getQuadrantWeights(QuadrantWeights& weights)
{
    const std::vector<FocusInformation>* const lists[2] = { &m_sensedFocus, &m_fireFocus };
    for (int l = 0; l < 2; ++l)
    {
        for (std::vector<FocusInformation>::const_iterator it = lists[l]->begin();
             it != lists[l]->end(); ++it)
        {
            int quads[2];
            if (!computeQuadrantsFromPosition(it->position, quads))
                continue;
            for (int i = 0; i < 2; ++i)
            {
                std::pair<float, std::pair<Vec3, unsigned int> >& entry = weights[quads[i]];
                entry.first += it->weight;
                entry.second.first.x += it->position.x;
                entry.second.first.y += it->position.y;
                entry.second.first.z += it->position.z;
                ++entry.second.second;
            }
        }
    }

    for (unsigned int q = 0; q <= 7; ++q)
    {
        std::pair<float, std::pair<Vec3, unsigned int> >& entry = weights[q];
        entry.first += m_quadrantWeights[q];
        if (entry.second.second)
        {
            const double scale = 1.0 / static_cast<double>(entry.second.second);
            entry.second.first.x = static_cast<float>(entry.second.first.x * scale);
            entry.second.first.y = static_cast<float>(entry.second.first.y * scale);
            entry.second.first.z = static_cast<float>(entry.second.first.z * scale);
        }
    }
}

// The heaviest feasible quadrant; failing one, up to seven random draws for a feasible one,
// which then weighs 0.1 at no position.
void SensingAgent::computeTargetQuadrant(const QuadrantWeights& weights,
                                         QuadrantInformation* quadrant)
{
    quadrant->quad = -1;
    quadrant->weight = 0.0f;
    float best = 0.0f;
    int q = 0;
    for (QuadrantWeights::const_iterator it = weights.begin(); it != weights.end(); ++it, ++q)
    {
        // the binary fetches the position and drops it
        Vec2 position;
        position.x = 0.0f;
        position.y = 0.0f;
        getQuadPosition(q, position);
        if (it->first > best && targetFeasible(q))
        {
            best = it->first;
            quadrant->weight = it->first;
            quadrant->quad = q;
            quadrant->position = it->second.first;
        }
    }
    if (quadrant->quad >= 0)
        return;

    quadrant->quad = static_cast<int>(static_cast<float>(std::floor(
        static_cast<double>(AIRandom::m_instance->next()) * 2.3283064365386963e-10 * 7.0 + 0.5)));
    int tries = 0;
    while (!targetFeasible(quadrant->quad) && tries <= 5)
    {
        ++tries;
        quadrant->quad = static_cast<int>(static_cast<float>(std::floor(
            static_cast<double>(AIRandom::m_instance->next()) * 2.3283064365386963e-10 * 7.0
            + 0.5)));
        Vec2 position;
        position.x = 0.0f;
        position.y = 0.0f;
        getQuadPosition(quadrant->quad, position);
    }
    quadrant->weight = 0.1f;
    quadrant->position.x = 0.0f;
    quadrant->position.y = 0.0f;
    quadrant->position.z = 0.0f;
}

// The next reaction time: short in vehicles, longer the less the bot moves and the more the
// chosen quadrant lies ahead of where it is going.
void SensingAgent::calculateCurrentPeriod(QuadrantInformation* quadrant, Information* equipment,
                                          const Vec2& direction)
{
    const io::PlayerInput* const input = m_bot->getPlayerInput();
    if (!equipment->getUnit())
        return;

    const unsigned int flags = equipment->getObjectTypeFlags();
    if (equipment->getUnit()->mustBorrowArmament() || (flags & 0x20) || (flags & 0x10))
    {
        m_reactionTime = randomInRange(1.0f, 1.5f);
        return;
    }
    if (direction.x * direction.x + direction.y * direction.y < 0.01f)
    {
        m_reactionTime = randomInRange(1.5f, 2.0f);
        return;
    }

    const bool soldier = (flags & 0x200000u) != 0;
    if (playerInput(input, io::PIThrottle) < 0.01f && playerInput(input, io::PIYaw) < 0.01f)
    {
        m_reactionTime = soldier ? randomInRange(3.0f, 4.0f) : randomInRange(7.0f, 10.0f);
        return;
    }

    Vec2 ahead;
    ahead.x = -direction.x;
    ahead.y = -direction.y;
    if (isForward(quadrant->quad, ahead))
        m_reactionTime = randomInRange(7.0f, 10.0f);
    else if (soldier)
        m_reactionTime = randomInRange(1.5f, 2.0f);
    else
        m_reactionTime = randomInRange(5.0f, 7.0f);
}

// A vehicle (or a borrowed weapon) always replans. Otherwise the plan goes when the quadrant
// stops being feasible or changes, when the plan target has become a sensed object (the quadrant
// logic then covers it), when a prone bot backs up, or when the bot has turned its back.
void SensingAgent::checkForPlanInvalidation(QuadrantInformation* quadrant, bool behind)
{
    bool invalid = false;
    Information* const equipment = informationManager->getInformation(m_bot->getCurrentEquipment());
    const unsigned int flags = equipment->getObjectTypeFlags();
    const bool vehicle = (flags & 0x20) || (flags & 0x10);
    if (equipment->getUnit()->mustBorrowArmament() || vehicle)
    {
        deletePlan();
        return;
    }

    if (!targetFeasible(m_currentQuad))
        invalid = true;
    if (m_lookingIntoQuadrant)
    {
        if (quadrant->quad != m_currentQuad)
        {
            m_currentQuadSeen = false;
            invalid = true;
        }
    }
    else if (m_bot->getSensedObjectFromHandle(m_planTarget))
    {
        invalid = true;
    }

    if (equipment->getSoldierPose() == 3 && playerInput(m_bot->getPlayerInput(), io::PIThrottle) < 0.0f)
        invalid = true;
    if (behind)
        invalid = true;

    if (m_plan && invalid)
        deletePlan();
}

// Vehicles look along the vehicle camera (pitched down for 0x10 and 0x20 equipment); on foot the
// plan looks into the chosen quadrant, or at the plan target/direction when m_lookDirection is
// empty and the plan direction outweighs the quadrant.
void SensingAgent::createPlan(ISensingAgentPattern* pattern, QuadrantInformation* quadrant)
{
    if (m_plan)
        m_plan->m_owner = pattern;

    BotMain* const bot = m_bot;
    // no null guard, as shipped
    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const unsigned int flags = equipment->getObjectTypeFlags();
    const bool vehicle = (flags & 0x20) || (flags & 0x10);

    if (equipment->getUnit()->mustBorrowArmament() || vehicle)
    {
        IAIObject* const object = m_bot->getPlayer()->getCurrentVehicle();
        const float dofOffset = equipment->getControlInfo()->getCameraRelativeDofRotationOffset();
        Vec3 forward = forwardRow(object->getTransformation());
        applyDofRotation(forward, dofOffset);
        if (vehicle)
            forward.y = (flags & 0x10) ? -0.4f : -0.7f;
        createDirPlan(forward, pattern);
        m_lookingForward = false;
        return;
    }

    if (m_lookDirection.x * m_lookDirection.x + m_lookDirection.y * m_lookDirection.y < 0.01f
        && m_planDirectionWeight > quadrant->weight)
    {
        if (m_planTarget != m_bot->getCurrentEquipment())
        {
            m_plan = pattern->createPlan(this, bot, m_planTarget);
            m_lookingIntoQuadrant = false;
            m_currentQuad = -1;
            m_currentQuadWeight = 0.0f;
        }
        else
        {
            Vec3 direction = m_planDirection;
            normalise(direction);
            m_plan = pattern->createPlan(this, bot, direction);
            m_lookingIntoQuadrant = false;
        }
        m_lookingForward = false;
    }
    else if (quadrant->quad < 0)
    {
        Debug(DebugTypeError, "AI/AIDLL/AI/AIBot/SensingAgent.cpp", 1066, "")
            << "Sensing agent plan has nothing to look at!";
    }
    else
    {
        Vec3 direction;
        direction.x = 0.0f;
        direction.y = 0.0f;
        direction.z = 0.0f;
        const Mat4& transformation = equipment->getTransformation();
        if (quadrant->position.y == 0.0f)
        {
            Vec2 position;
            position.x = 0.0f;
            position.y = 0.0f;
            getQuadPosition(quadrant->quad, position);
            direction.x = position.x;
            direction.y = transformation.m[9];
            direction.z = position.y;
        }
        else
        {
            direction.x = quadrant->position.x - transformation.m[12];
            direction.y = quadrant->position.y - transformation.m[13];
            direction.z = quadrant->position.z - transformation.m[14];
        }
        normalise(direction);

        m_currentQuad = quadrant->quad;
        m_currentQuadWeight = quadrant->weight;
        m_plan = pattern->createPlan(this, bot, direction);
        m_lookingIntoQuadrant = true;
        m_currentQuad = quadrant->quad;
        m_currentQuadWeight = quadrant->weight;
        m_currentQuadPosition = quadrant->position;
        m_lookingForward = isForward(m_currentQuad, m_lookDirection);
    }

    if (m_plan)
        m_plan->m_owner = this;
}

// Clears the plan target and the weights, not the entries, of both focus lists.
void SensingAgent::clearInformation()
{
    clearPlanTargetInformation();
    for (std::vector<FocusInformation>::iterator it = m_sensedFocus.begin();
         it != m_sensedFocus.end(); ++it)
    {
        it->weight = 0.0f;
    }
    for (std::vector<FocusInformation>::iterator it = m_fireFocus.begin();
         it != m_fireFocus.end(); ++it)
    {
        it->weight = 0.0f;
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice
