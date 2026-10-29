// Control-information accessors: each one forwards to the engine object's control-info plug-in.

#include "IPIControlInfo.h"

#include "Information.h"
#include "../../../AIInterface/BFKnowledge/IAIObject.h"
#include "../../../AIInterface/BFKnowledge/IAIObjectPlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

unsigned long long IPIControlInfo::getLookControls() const
{
    return m_information->getAIObject()->getControlInfo()->getLookControls();
}

io::PlayerInputMap IPIControlInfo::getLookVerticalControl() const
{
    return m_information->getAIObject()->getControlInfo()->getLookVerticalControl();
}

io::PlayerInputMap IPIControlInfo::getLookHorizontalControl() const
{
    return m_information->getAIObject()->getControlInfo()->getLookHorizontalControl();
}

float IPIControlInfo::getProportionalConstant() const
{
    return m_information->getAIObject()->getControlInfo()->getProportionalConstant();
}

float IPIControlInfo::getDerivativeConstant() const
{
    return m_information->getAIObject()->getControlInfo()->getDerivativeConstant();
}

float IPIControlInfo::getAngleSpeedConstant() const
{
    return m_information->getAIObject()->getControlInfo()->getAngleSpeedConstant();
}

float IPIControlInfo::getCameraRelativeDofRotationOffset() const
{
    return m_information->getAIObject()->getControlInfo()->getCameraRelativeDofRotationOffset();
}

const Vec3& IPIControlInfo::getCameraRelativeMinRotation() const
{
    return m_information->getAIObject()->getControlInfo()->getCameraRelativeMinRotation();
}

const Vec3& IPIControlInfo::getCameraRelativeMaxRotation() const
{
    return m_information->getAIObject()->getControlInfo()->getCameraRelativeMaxRotation();
}

void IPIControlInfo::getCameraRelativeRotation(Vec3& out) const
{
    m_information->getAIObject()->getControlInfo()->getCameraRelativeRotation(out);
}

const Mat4& IPIControlInfo::getCameraTransformation() const
{
    return m_information->getAIObject()->getControlInfo()->getCameraTransformation();
}

void IPIControlInfo::getCameraBaseTransformation(Mat4& out) const
{
    m_information->getAIObject()->getControlInfo()->getCameraBaseTransformation(out);
}

bool IPIControlInfo::validateCameraDirection(const Vec3& direction) const
{
    return m_information->getAIObject()->getControlInfo()->validateCameraDirection(direction);
}

} // namespace ai
} // namespace hfe
} // namespace dice
