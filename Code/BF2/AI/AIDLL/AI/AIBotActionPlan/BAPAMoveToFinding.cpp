#include "BAPAMoveToFinding.h"

#include "BAPCondition.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/BotNavigation.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// 0x1e96e0 (Ghidra). The bot's equipment is asked for and dropped, as shipped.
BAPAMoveToFinding::BAPAMoveToFinding(Bot* bot, const Vec3& position, float maxSpeed, float maxArriveSpeed,
                                     BAPCondition* finishCondition, unsigned long long controls, int planId,
                                     float radius, float minRadius)
    : BAPAMoveTo(bot)
    , m_boolD4(false)
    , m_boolD5(false)
    , m_intD8(1)
    , m_bot(bot)
    , m_radius(radius)
    , m_minRadius(minRadius)
    , m_radiusSqr(radius * radius)
    , m_minRadiusSqr(minRadius * minRadius)
    , m_boolF8(false)
{
    for (int i = 0; i < 8; ++i)
        m_floatB4[i] = 0.0f;
    bot->getCurrentEquipment();
    m_planId = planId;
    m_coordinate = position;
    m_maxSpeed = maxSpeed;
    m_maxArriveSpeed = maxArriveSpeed;
    m_finishCondition = finishCondition;
    if (finishCondition)
        finishCondition->m_owner = this;
    m_float60 = 0.0f;
    m_hasPath = true;
    m_pathStarted = false;
    m_controls = controls;
    m_validPoint.x = 0.0f;
    m_validPoint.y = 0.0f;
}

BAPAMoveToFinding::~BAPAMoveToFinding()
{
}

// 0x1e9af0 (Ghidra). Runs only through an interpreter entry.
bool BAPAMoveToFinding::execute(Bot*, bool& ok, bool& done)
{
    ok = false;
    done = true;
    return false;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton] Debugger output only.
void BAPAMoveToFinding::streamSelf(std::ostream&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPAMoveToFinding::streamSelf");
}

// [FRONTIER] 0x1e9b10 (Ghidra), 1.9 KB: starts the path search (BotNavigation::initPathfinding),
// follows it (updatePath), and near the target looks around it for a valid position.
bool BAPAMoveToFinding::update(Vec2&, bool&, bool&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPAMoveToFinding::update");
}

// 0x1ea3b0 (Ghidra). Finishing clears the navigation's progress.
bool BAPAMoveToFinding::isFinished(Bot* bot)
{
    if ((!m_finishCondition || !m_finishCondition->evaluate(bot)) && !withinDistance())
        return false;
    static_cast<BotMain*>(bot)->getBotNavigation()->clearProgress();
    return true;
}

bool BAPAMoveToFinding::getSecondPoint(Vec2& point)
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->getSecondPoint(point);
}

bool BAPAMoveToFinding::getLargestAngleDiff(float& angle)
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->getLargestAngleDiff(angle);
}

float BAPAMoveToFinding::getTargetDistance()
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->getClosestDistance();
}

bool BAPAMoveToFinding::usesPathfinding()
{
    return true;
}

// 0x1ea360 (Ghidra). Between the two radii, both included.
bool BAPAMoveToFinding::withinDistance()
{
    const float distanceSqr = getSimpleTargetDistanceSqr(m_bot);
    return distanceSqr <= m_radiusSqr && m_minRadiusSqr <= distanceSqr;
}

// [UNRESOLVED] The navigation's +192 flag, which BotNavigation.h names isPathSearchPending; read here
// as the stuck flag (setIsStuck writes it).
bool BAPAMoveToFinding::isStuck()
{
    return static_cast<BotMain*>(m_bot)->getBotNavigation()->isPathSearchPending();
}

} // namespace ai
} // namespace hfe
} // namespace dice
