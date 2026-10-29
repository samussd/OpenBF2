#include "InfanteryControl.h"

#include "../AIBot/BotMain.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../../../../IO/Input/PlayerInput.h"
#include "../../../../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace ai {

// The look group reads the equipment without a null guard: element 9 of its transformation (the
// forward vector's y) is negated into the vertical look.
void infanteryResetControls(Bot* bot, io::PlayerInput* input, bool movement, bool look, bool actions)
{
    if (movement)
    {
        input->setInput(io::PIAction, 0.0f);
        input->setInput(io::PISprint, 0.0f);
        input->setInput(io::PIYaw, 0.0f);
        input->setInput(io::PIRoll, 0.0f);
        input->setInput(io::PIPitch, 0.0f);
        input->setInput(io::PIThrottle, 0.0f);
        input->setInput(io::PIMouseLookX, 0.0f);
        input->setInput(io::PIMouseLookY, 0.0f);
    }
    if (look)
    {
        // [PLATFORM] ELF captures the manager before the equipment callback.
        InformationManager* const manager = informationManager;
        Information* const equipment = manager->getInformation(bot->getCurrentEquipment());
        const Mat4& transformation = equipment->getTransformation();
        input->setInput(io::PIMouseLookX, 0.0f);
        input->setInput(io::PIMouseLookY, -transformation.m[9]);
    }
    if (actions)
    {
        input->setInput(io::PIFire, 0.0f);
        input->setInput(io::PIAltFire, 0.0f);
        input->setInput(io::PIWeaponSelect1, 0.0f);
        input->setInput(io::PIWeaponSelect2, 0.0f);
        input->setInput(io::PIWeaponSelect3, 0.0f);
        input->setInput(io::PIWeaponSelect4, 0.0f);
        input->setInput(io::PIWeaponSelect5, 0.0f);
        input->setInput(io::PIWeaponSelect6, 0.0f);
        input->setInput(io::PIWeaponSelect7, 0.0f);
        input->setInput(io::PIWeaponSelect8, 0.0f);
        input->setInput(io::PIPositionSelect1, 0.0f);
        input->setInput(io::PIPositionSelect2, 0.0f);
        input->setInput(io::PIPositionSelect3, 0.0f);
        input->setInput(io::PIPositionSelect4, 0.0f);
        input->setInput(io::PIPositionSelect5, 0.0f);
        input->setInput(io::PIPositionSelect6, 0.0f);
        input->setInput(io::PIPositionSelect7, 0.0f);
        input->setInput(io::PIPositionSelect8, 0.0f);
        input->setInput(io::PIUse, 0.0f);
        input->setInput(io::PIReload, 0.0f);
        input->setInput(io::PIFlareFire, 0.0f);
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice
