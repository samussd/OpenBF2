// Control information plug-in; adds no virtual methods to InformationPlugIn. Its getters read
// through to the engine object's control-info plug-in on every call.
// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding IPIControlInfo operation.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPICONTROLINFO_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPICONTROLINFO_H

#include "InformationPlugIn.h"

#include "../../../../Math/Vec3.h"

namespace dice { namespace hfe {

struct Mat4;

namespace ai {

class IPIControlInfo : public InformationPlugIn
{
public:
    explicit IPIControlInfo(Information* information) { m_information = information; }

    // only the base's vptr store - nothing of its own.
    virtual ~IPIControlInfo() {}
    // 5 - Information::removePlugIn's case for the control-info plug-in.
    virtual int getType() { return 5; }
    // empty - the control values are read
    // through to the engine object on every call, so there is nothing to refresh.
    virtual void setTime() {}
    virtual void setTime(double /*time*/) {}

    unsigned long long getLookControls() const;
    // the camera block (IAIObjectControlInfo +0x94..+0xBC), one forward each.
    float getCameraRelativeDofRotationOffset() const;
    const Vec3& getCameraRelativeMinRotation() const;
    const Vec3& getCameraRelativeMaxRotation() const;
    void getCameraRelativeRotation(Vec3& out) const;
    const Mat4& getCameraTransformation() const;
    void getCameraBaseTransformation(Mat4& out) const;
    bool validateCameraDirection(const Vec3& direction) const;
};

} } }
#endif
