#include "BAPAComponentSoldierPose.h"

#include <cfloat>

#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AIRandom.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"

namespace dice {
namespace hfe {
namespace ai {

// The fire threshold is drawn once: threshold + r * 5 * threshold for r in [0, 1). Retail scales
// in x87 precision before storing the float.
BAPAComponentSoldierPose::BAPAComponentSoldierPose(SoldierPose normalPose,
                                                   SoldierPose incommingFirePose,
                                                   float fireObjectsThreshold,
                                                   SoldierPose dangerPose, float controlThreshold,
                                                   unsigned int target, float timeDuration)
    : m_normalPose(normalPose)
    , m_incommingFirePose(incommingFirePose)
    , m_dangerPose(dangerPose)
    , m_currentPose(normalPose)
    , m_timeDuration(timeDuration)
    , m_double20(-FLT_MAX)
    , m_target(target)
{
    const float spread = fireObjectsThreshold * 5.0f;
    m_fireObjectsThreshold = static_cast<float>(AIRandom::m_instance->next()) * 2.3283064e-10f * spread
                             + fireObjectsThreshold;
    // Unordered clamps to -1, as retail's first comparison does.
    if (!(-1.0f <= controlThreshold))
        m_controlThreshold = -1.0f;
    else
        m_controlThreshold = 1.0f < controlThreshold ? 1.0f : controlThreshold;
}

BAPAComponentSoldierPose::~BAPAComponentSoldierPose()
{
}

void BAPAComponentSoldierPose::setNormalPose(SoldierPose pose)
{
    m_normalPose = pose;
}

void BAPAComponentSoldierPose::setIncommingFirePose(SoldierPose pose)
{
    m_incommingFirePose = pose;
}

void BAPAComponentSoldierPose::setFireObjectsThreshold(float threshold)
{
    m_fireObjectsThreshold = threshold;
}

void BAPAComponentSoldierPose::setDangerPose(SoldierPose pose)
{
    m_dangerPose = pose;
}

void BAPAComponentSoldierPose::setControlThreshold(float threshold)
{
    m_controlThreshold = threshold;
}

void BAPAComponentSoldierPose::setTimeDuration(float duration)
{
    m_timeDuration = duration;
}

SoldierPose BAPAComponentSoldierPose::getNormalPose()
{
    if (m_target != 0xFFFFFFFFu)
    {
        Information* const target = informationManager->getInformation(m_target);
        if (target && (target->getObjectTypeFlags() & 0x200000u))
        {
            IAIObject* const object = target->getAIObject();
            IAIPlayer* const player =
                static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
                    ->getPlayerFromId(object->getPlayerId());
            if (player && player->getSoldier() && object->getSoldierPose() != SPStanding)
                return SPCrouching;
        }
    }
    return m_normalPose;
}

SoldierPose BAPAComponentSoldierPose::getIncommingFirePose() const
{
    return m_incommingFirePose;
}

float BAPAComponentSoldierPose::getFireObjectsThreshold() const
{
    return m_fireObjectsThreshold;
}

SoldierPose BAPAComponentSoldierPose::getDangerPose() const
{
    return m_dangerPose;
}

float BAPAComponentSoldierPose::getControlThreshold() const
{
    return m_controlThreshold;
}

float BAPAComponentSoldierPose::getTimeDuration() const
{
    return m_timeDuration;
}

SoldierPose BAPAComponentSoldierPose::getCurrentPose() const
{
    return m_currentPose;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
SoldierPose BAPAComponentSoldierPose::computeCurrentPose(float, float, double, Bot*, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BAPAComponentSoldierPose::computeCurrentPose");
}

} // namespace ai
} // namespace hfe
} // namespace dice
