// Physical information a side remembers. Shape questions go straight to the live object; health
// and ground state are as they were at the last sighting.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPIPHYSICALKNOWN_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPIPHYSICALKNOWN_H

#include "IPIPhysical.h"

namespace dice {
namespace hfe {
namespace ai {

class IPIPhysicalKnown : public IPIPhysical
{
public:
    explicit IPIPhysicalKnown(Information* information);
    virtual ~IPIPhysicalKnown();

    virtual void   setTime();                                           // +3
    virtual void   setTime(double time);                                // +4
    virtual float  getHealth() const;                                   // +5
    virtual void   setHealth(float health);                             // +6
    virtual float  getMaxHealth();                                      // +7
    virtual float  getMod();                                            // +8
    virtual BoundingBox*            getBoundingBox();                   // +9
    virtual const LocalBoundingBox* getLocalBoundingBox();              // +10
    virtual float  getRadius();                                         // +11
    virtual Vec3   getSphereLocalOffset() const;                        // +12
    virtual float  getSmallestRadius() const;                           // +13
    virtual void   getSubSpheres2d(std::vector<Vec2>& out) const;       // +14
    virtual float  getArmour(Vec3& direction);                          // +15
    virtual float  getBestArmour();                                     // +16
    virtual int    getStrengthType();                                   // +17
    virtual float  getVolume();                                         // +18
    virtual float  getMass();                                           // +19
    virtual int    getMaterial() const;                                 // +20
    virtual float  getManDownTimeLeft() const;                          // +21
    virtual float  getMaxManDownTime() const;                           // +22
    virtual bool   getManDown() const;                                  // +23
    virtual bool   isTouchingTerrain(Vec3& position) const;             // +24
    virtual bool   isInAir() const;                                     // +25
    virtual bool   isInWater() const;                                   // +26

private:
    /* +0x08 */ float m_mod;
    /* +0x0C */ float m_health;
    /* +0x10 */ bool  m_touchingTerrain;
    /* +0x14 */ Vec3  m_terrainPosition;
    /* +0x20 */ bool  m_inAir;
    /* +0x21 */ bool  m_inWater;
    /* +0x22 */ bool  m_manDown;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_IPIPHYSICALKNOWN_H
