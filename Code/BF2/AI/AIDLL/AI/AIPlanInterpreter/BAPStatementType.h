// The plan-script statement types. Values are the ELF reader's; 30 has no script name.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_BAPSTATEMENTTYPE_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_BAPSTATEMENTTYPE_H

namespace dice {
namespace hfe {
namespace ai {

enum BAPStatementType : int
{
    BAPSTParallel = 0,
    BAPSTSerial = 1,
    BAPSTPlanEnd = 2,
    BAPSTIf = 3,
    BAPSTWait = 4,
    BAPSTWhile = 5,
    BAPSTAimAt = 6,
    BAPSTMoveTo = 7,
    BAPSTMoveToDirection = 8,
    BAPSTMoveToObject = 9,
    BAPSTMoveAwayFromObject = 10,
    BAPSTTrigger = 11,
    BAPSTTriggerContinously = 12,
    BAPSTIdle = 13,
    BAPSTResetControls = 14,
    BAPSTInfoWrapper = 15,
    BAPSTAimingWrapper = 16,
    BAPSTFinishedWrapper = 17,
    BAPSTWeaponWrapper = 18,
    BAPSTLookAtWrapper = 19,
    BAPSTSense = 20,
    BAPSTOverrideSensingAgent = 21,
    BAPSTPlaceC4 = 22,
    BAPSTLookAt = 23,
    BAPSTTurn = 24,
    BAPSTRoll3d = 25,
    BAPSTSoldierPose = 26,
    BAPSTUpdateVehicle = 27,
    BAPSTChangeWeapon = 28,
    BAPSTICChangeVehicle = 29,
    BAPSTICTurnLock = 31,
    BAPSTEvaluate = 32,
    BAPSTNotifyBot = 33,
    BAPSTPhysicsEnable = 34,
    BAPSTSetSensingAgentFocus = 35,
    BAPSTMergeSoldierInputs = 36,
    BAPSTSendRadioMessage = 37,
    BAPSTDebug = 38,
    BAPSTForgetTarget = 39,
    BAPSTUseFlareOnMissileLock = 40,
    BAPSTRunOverObject = 41,
    BAPSTHonkHorn = 42,
    BAPSTEvaluateCondition = 43,
    BAPSTRevive = 44,
    BAPSTNumberOfTypes = 45      // what the reader stores for an unknown name
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_BAPSTATEMENTTYPE_H
