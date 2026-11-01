// What a side remembers about an object it has seen. While nobody is in contact the position and
// transformation stay as they were last seen, and the side's certainty fades with time.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONKNOWN_H
#define AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONKNOWN_H

#include "Information.h"
#include "../../../../Math/Mat4.h"
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class InformationKnown : public Information
{
public:
    InformationKnown(IAIObject* object, unsigned int types, double time, float degeneration,
                     float basicTemp, float allowedTimeDiff, int side);
    virtual ~InformationKnown();

    virtual int    getInformationSort();                                // +2 - 1
    virtual bool   setTime(double time);                                // +3
    virtual bool   setTime(double time, float security);                // +4
    virtual double getTimeStamp() const;                                // +5
    virtual void   addContact();                                        // +6
    virtual void   removeContact();                                     // +7
    virtual void   removeAllContacts();                                 // +8
    virtual bool   hasContact() const;                                  // +9
    virtual float  getSecurity(double time);                            // +10
    virtual float  getSecurity();                                       // +11
    virtual Vec3&  getPosition();                                       // +12
    virtual float  getAltitude();                                       // +13
    virtual float  getAltitude(const Vec3& direction);                  // +14
    virtual Mat4&  getTransformation();                                 // +15
    virtual Vec3   getFaceing();                                        // +16
    virtual int    getSide() const;                                     // +17

    int getOwnerSide() const { return m_ownerSide; }
    void setOwnerSide(int side) { m_ownerSide = side; }

private:
    void rememberObject();
    void forEachSecondary(void (Information::*action)());

    /* +0x44 */ double m_timeStamp;
    /* +0x4C */ float  m_degeneration;
    /* +0x50 */ Vec3   m_lastPosition;
    /* +0x5C */ Mat4   m_lastTransformation;
    /* +0x9C */ int    m_side;              // the object's own side
    /* +0xA0 */ int    m_ownerSide;         // the side that knows it
    /* +0xA4 */ int    m_contacts;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONKNOWN_H
