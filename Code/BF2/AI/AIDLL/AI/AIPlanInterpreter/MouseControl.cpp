#include "MouseControl.h"

#include "../AIBot/BotMain.h"                    // Bot
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIControlInfo.h"
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../../IO/Input/PlayerInput.h"
#include "../../../../Math/Mat4.h"
#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

namespace {

float clampUnit(float value)
{
    if (value < -1.0f)
        return -1.0f;
    if (value > 1.0f)
        return 1.0f;
    return value;
}

// 1, -1 or 0.
float sign(float value)
{
    if (value > 0.0f)
        return 1.0f;
    if (value < 0.0f)
        return -1.0f;
    return 0.0f;
}

float axis(const io::PlayerInput* input, io::PlayerInputMap which)
{
    return (input->mappingFlags >> which) & 1 ? input->inputs[which] : 0.0f;
}

} // namespace

// The binary's function statics are kept as such; they are plain constants in practice.
void mouseControlLookAtDirection(Bot* bot, const Vec3& direction, const Mat4& camera,
                                 float& pitchError, float previousPitchError,
                                 float& yawError, float previousYawError,
                                 float allowedDeviation, IPIControlInfo* control,
                                 io::PlayerInput* input, float bias, float yawSpeed,
                                 float pitchSpeed)
{
    static bool useAllowedDeviation = false;
    static float minInput = 0.01f;

    const float* const m = camera.m;
    const float up = clampUnit(m[4] * direction.x + m[5] * direction.y + m[6] * direction.z);
    const float right = clampUnit(m[0] * direction.x + m[1] * direction.y + m[2] * direction.z);
    const float forward = clampUnit(m[8] * direction.x + m[9] * direction.y + m[10] * direction.z);

    pitchError = -(std::acos(up) - 1.5707964f);
    const float rightAngle = std::acos(right);
    if (forward < 0.0f)
    {
        if (right < 0.0f)
            yawError = 4.712389f - rightAngle;
        else
            yawError = -(rightAngle + 1.5707964f);
    }
    else
    {
        yawError = rightAngle - 1.5707964f;
    }

    const float proportional = control->getProportionalConstant();
    float yawProportional = proportional;
    const float derivative = control->getDerivativeConstant();
    const float angleSpeed = control->getAngleSpeedConstant();
    float maxX = 4.0f;
    float maxY = 4.0f;

    Information* const equipment = informationManager->getInformation(bot->getCurrentEquipment());
    const float skill =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getBotSkill();
    if (!equipment || !(equipment->getObjectTypeFlags() & 0x200000u))
    {
        static float maxXInputNew = 1.0f;
        static float maxYInputNew = 2.0f;
        maxX = (maxX - maxXInputNew) * skill + maxXInputNew;
        maxY = (maxY - maxYInputNew) * skill + maxYInputNew;
    }
    else if (bot->getFiringTarget() == 0xFFFFFFFFu && axis(input, io::PIThrottle) == 0.0f
             && axis(input, io::PISprint) == 0.0f && axis(input, io::PIYaw) == 0.0f)
    {
        // a soldier standing still with nothing to shoot at turns harder and slower
        static float newP = -3.0f;
        static float maxXInputNew = 1.5f;
        yawProportional = newP;
        maxX = maxXInputNew;
    }
    else if (axis(input, io::PISprint) == 0.0f)
    {
        static float maxXInputNew = 1.0f;
        maxX = (maxX - maxXInputNew) * skill + maxXInputNew;
    }

    // pitch
    float speedTerm = angleSpeed * pitchSpeed;
    if (sign(pitchError) == sign(pitchSpeed))
        speedTerm *= 0.5f;
    float biasTerm = 0.0f;
    if (bias < -0.0001f)
    {
        static float tweak = 0.05f;
        biasTerm = bias / -tweak;
        if (!(biasTerm < 0.75f))
            biasTerm = 0.75f;
    }
    float pitch = proportional * pitchError + (pitchError - previousPitchError) * derivative
                  + speedTerm + biasTerm;
    if (!useAllowedDeviation || !(allowedDeviation > std::fabs(pitchError)))
    {
        if (std::fabs(pitch) > 4.0f)
            pitch = 4.0f * sign(pitch);
    }
    else
    {
        pitch = 0.0f;
    }
    pitch = pitch * (1.0f - skill * 0.5f) * 0.5f;
    if (std::fabs(pitch) < minInput && pitch != 0.0f)
        pitch = sign(pitch) * minInput;
    if (pitch < -maxY)
        pitch = -maxY;
    else if (pitch > maxY)
        pitch = maxY;
    input->setInput(control->getLookVerticalControl(), pitch);

    // yaw
    float yawSpeedTerm = angleSpeed * yawSpeed;
    if (sign(yawError) == sign(yawSpeed))
        yawSpeedTerm *= 0.5f;
    float yaw = yawProportional * yawError + (yawError - previousYawError) * derivative
                + yawSpeedTerm;
    if (!useAllowedDeviation || !(allowedDeviation > std::fabs(yawError)))
    {
        if (std::fabs(yaw) > 4.0f)
            yaw = 4.0f * sign(yaw);
    }
    else
    {
        yaw = 0.0f;
    }
    if (std::fabs(yaw) < minInput && yaw != 0.0f)
        yaw = sign(yaw) * minInput;
    if (yaw < -maxX)
        yaw = -maxX;
    else if (yaw > maxX)
        yaw = maxX;
    input->setInput(control->getLookHorizontalControl(), yaw);
}

} // namespace ai
} // namespace hfe
} // namespace dice
