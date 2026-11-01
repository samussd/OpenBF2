
#include "WeaponInfoKnown.h"

#include "../AIBotBehaviour/CustomCurve.h"                          // SCurve::calculate
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice { namespace hfe { namespace ai {

namespace {

double gameTime()
{
    return static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
}

} // anonymous namespace

WeaponInfoKnown::WeaponInfoKnown(IWeapon* weapon)
    : WeaponInfo(weapon)
{
    read();
    m_timeStamp = gameTime();
}

WeaponInfoKnown::~WeaponInfoKnown()
{
}

void WeaponInfoKnown::read()
{
    m_ammo = m_weapon->getAmmo();
    m_currentAmmo = m_weapon->getCurrentAmmo();
    m_readyToFire = m_weapon->isReadyToFire();
    m_heat = m_weapon->getHeat();
    m_timeOfFiring = m_weapon->getTimeOfFiring();
    m_shotsInMag.clear();
    for (int magazine = 0; magazine < m_weapon->getNumberOfMags(); ++magazine)
        m_shotsInMag.push_back(m_weapon->getAmmoInMag(magazine));
}

bool WeaponInfoKnown::update()
{
    const double now = gameTime();
    if (1.0f + static_cast<float>(m_timeStamp) < static_cast<float>(now))
    {
        read();
        m_timeStamp = now;
    }
    return true;
}

int WeaponInfoKnown::getShotsInMag(int magazine) { return m_shotsInMag[magazine]; }
int WeaponInfoKnown::getAmmo() const { return m_ammo; }
int WeaponInfoKnown::getCurrentAmmo() const { return m_currentAmmo; }
int WeaponInfoKnown::getNumberOfMags() const { return static_cast<int>(m_shotsInMag.size()); }
bool WeaponInfoKnown::isReadyToFire() const { return m_readyToFire; }
double WeaponInfoKnown::getTimeOfFiring() const { return m_timeOfFiring; }
float WeaponInfoKnown::getHeat() const { return m_heat; }
bool WeaponInfoKnown::usesHeat() const { return m_weapon->usesHeat(); }

float WeaponInfoKnown::getSecurity() const
{
    return getSecurity(gameTime());
}

// Certainty fades over five seconds.
float WeaponInfoKnown::getSecurity(double time) const
{
    return 1.0f - SCurve::calculate(static_cast<float>((time - m_timeStamp) * 0.2));
}

} } }
