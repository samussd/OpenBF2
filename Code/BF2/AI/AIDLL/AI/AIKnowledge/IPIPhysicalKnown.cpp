
#include "IPIPhysicalKnown.h"

#include "Information.h"
#include "../AIBotBehaviour/CustomCurve.h"                          // SCurve::calculate
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"
#include "../../../AIInterface/BFKnowledge/IAIObjectPlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

IPIPhysicalKnown::IPIPhysicalKnown(Information* information)
    : m_mod(0.0f)
    , m_health(0.0f)
    , m_touchingTerrain(false)
    , m_inAir(false)
    , m_inWater(false)
    , m_manDown(false)
{
    m_information = information;
    m_terrainPosition.x = 0.0f;
    m_terrainPosition.y = 0.0f;
    m_terrainPosition.z = 0.0f;
}

IPIPhysicalKnown::~IPIPhysicalKnown()
{
}

void IPIPhysicalKnown::setTime()
{
    setTime(static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime());
}

void IPIPhysicalKnown::setTime(double /*time*/)
{
    IAIObjectPhysical* const physical = m_information->getAIObject()->getPhysical();
    m_manDown = physical->getManDown();
    m_health = physical->getMaxHealth() > 0.0f ? physical->getHealth() : 0.0f;
    m_mod = 0.5f;
    m_mod = SCurve::calculate(m_health) * 0.5f + m_mod;
    m_touchingTerrain = physical->isTouchingTerrain(m_terrainPosition);
    m_inAir = physical->isInAir();
    m_inWater = physical->isInWater();
}

float IPIPhysicalKnown::getHealth() const
{
    return m_health;
}

void IPIPhysicalKnown::setHealth(float health)
{
    m_health = health;
}

float IPIPhysicalKnown::getMaxHealth()
{
    return m_information->getAIObject()->getPhysical()->getMaxHealth();
}

float IPIPhysicalKnown::getMod()
{
    return m_mod;
}

BoundingBox* IPIPhysicalKnown::getBoundingBox()
{
    return m_information->getAIObject()->getPhysical()->getCurrentBoundingBox();
}

const LocalBoundingBox* IPIPhysicalKnown::getLocalBoundingBox()
{
    return m_information->getAIObject()->getPhysical()->getLocalBoundingBox();
}

float IPIPhysicalKnown::getRadius()
{
    return m_information->getAIObject()->getPhysical()->getRadius();
}

Vec3 IPIPhysicalKnown::getSphereLocalOffset() const
{
    return m_information->getAIObject()->getPhysical()->getSphereLocalOffset();
}

float IPIPhysicalKnown::getSmallestRadius() const
{
    return m_information->getAIObject()->getPhysical()->getSmallestRadius();
}

void IPIPhysicalKnown::getSubSpheres2d(std::vector<Vec2>& out) const
{
    m_information->getAIObject()->getPhysical()->getSubSpheres2d(out);
}

float IPIPhysicalKnown::getArmour(Vec3& direction)
{
    return m_information->getAIObject()->getPhysical()->getArmour(direction);
}

float IPIPhysicalKnown::getBestArmour()
{
    return m_information->getAIObject()->getPhysical()->getBestArmour();
}

int IPIPhysicalKnown::getStrengthType()
{
    return m_information->getAIObject()->getPhysical()->getStrType();
}

float IPIPhysicalKnown::getVolume()
{
    return m_information->getAIObject()->getPhysical()->getVolume();
}

float IPIPhysicalKnown::getMass()
{
    return m_information->getAIObject()->getPhysical()->getMass();
}

int IPIPhysicalKnown::getMaterial() const
{
    return m_information->getAIObject()->getPhysical()->getMaterial();
}

float IPIPhysicalKnown::getManDownTimeLeft() const
{
    return m_information->getAIObject()->getPhysical()->getManDownTimeLeft();
}

float IPIPhysicalKnown::getMaxManDownTime() const
{
    return m_information->getAIObject()->getPhysical()->getMaxManDownTime();
}

bool IPIPhysicalKnown::getManDown() const
{
    return m_manDown;
}

bool IPIPhysicalKnown::isTouchingTerrain(Vec3& position) const
{
    position = m_terrainPosition;
    return m_touchingTerrain;
}

bool IPIPhysicalKnown::isInAir() const
{
    return m_inAir;
}

bool IPIPhysicalKnown::isInWater() const
{
    return m_inWater;
}

} // namespace ai
} // namespace hfe
} // namespace dice
