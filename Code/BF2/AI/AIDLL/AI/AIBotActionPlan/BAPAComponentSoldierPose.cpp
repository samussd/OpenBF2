#include "BAPAComponentSoldierPose.h"

#include <cfloat>

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../Main/AIRandom.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"
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

// 0x1e1940 (Ghidra). A swimmer stays one. At most once per m_timeDuration: crouches with a target
// that is not standing; otherwise the incoming-fire pose above that threshold, the normal pose at or
// above the control threshold and the danger pose below it. Only kits 1 and 3 lie down (the others
// crouch). Over 0.4 m of water at the position the bot stands; in any water it does not lie down.
SoldierPose BAPAComponentSoldierPose::computeCurrentPose(float fire, float control, double time, Bot* bot,
                                                         const Vec3& position)
{
    // [PLATFORM] ELF captures the manager before the equipment callback.
    InformationManager* const manager = informationManager;
    Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
    if (!equipment)
        return m_currentPose;
    if (equipment->getSoldierPose() == SPSwimming)
    {
        m_currentPose = SPSwimming;
        return SPSwimming;
    }
    if (time - m_double20 < m_timeDuration)
        return m_currentPose;
    m_double20 = time;
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
            {
                m_currentPose = SPCrouching;
                return SPCrouching;
            }
        }
    }
    SoldierPose pose;
    if (fire > m_fireObjectsThreshold)
        pose = m_incommingFirePose;
    else if (m_controlThreshold <= control)
        pose = m_normalPose;
    else
        pose = m_dangerPose;
    if (pose == SPLying && equipment->getKitType() != 1 && equipment->getKitType() != 3)
        pose = SPCrouching;
    m_currentPose = pose;
    const IAITerrain* const terrain = static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
    const float water = terrain->getWaterLevel(position.x, position.z);
    const float depth = water - terrain->getHeight(position.x, position.z);
    if (depth > 0.4f)
    {
        m_currentPose = SPStanding;
        return SPStanding;
    }
    if (depth > 0.0f && m_currentPose == SPLying)
    {
        m_currentPose = SPCrouching;
        return SPCrouching;
    }
    return m_currentPose;
}

} // namespace ai
} // namespace hfe
} // namespace dice
