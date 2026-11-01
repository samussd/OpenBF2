// Squad member state and coordinator association.
// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadMember operations.

#ifndef AI_AIDLL_AI_AIBOT_SQUADMEMBER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_SQUADMEMBER_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class IAIPlayer;
class Bot;
class Information;
class SquadCoordinator;
class WaypointList;

class SquadMember
{
public:
    SquadMember(SquadCoordinator* squad, IAIPlayer* player,
                int squadIndex, int side, int memberIndex);
    ~SquadMember();

    IAIPlayer* getPlayer();
    SquadCoordinator* getSquad();
    int getPlayerId();
    bool isBot();
    Bot* getBot();                           // borrowed
    int getMemberIndex();
    int getSquadId();                        // returns m_squadIndex
    // Whether the member's current request is this action on this handle.
    bool isSameRequest(unsigned int handle, int action);
    void setMemberIndex(int index);          // the squad renumbers members after a removal
    void reset();                            // round restart: no position, no movement state
    void squadMemberKilled();
    void setIsIdle(bool idle);               // an idle plan holds the member idle
    bool getIsIdle();
    void clearRequestAction();               // no action, no handle
    void setIsSquadLeader(bool leader);
    bool isSquadLeader();
    bool isMoving();                         // returns m_moved
    bool isAtFormationSlot();                // returns m_flag30
    float getMaxSpeed() const { return m_maxSpeed; }
    float getEstimatedArrival() const { return m_estimatedArrival; }
    bool isAvoidingArtillery();              // returns m_flag4B
    void setHavePath(bool havePath);
    void update(double time);                // bounded dependent calls
    void updateFormationStatus();
    bool getPosition(Vec3& position);         // copies even if invalid
    bool isAllowedInFormation();
    void updateMemberPos();
    bool isMobile();
    // On a mission to its assigned area, which the side does not own, and within the formation
    // distance of the squad's goal. The (float, float) overload is not called by this path.
    bool isCloseToNonOwnedTarget() const;
    void moveWithFormation();
    void moveWithoutFormation();
    float getVehicleRadius();
    int getSide() const { return m_side + 1; }     // one-based, unlike m_side
    // Gives the bot a route to the formation slot or to the squad's goal.
    void moveToSlotPos(const Vec3& slot, Information* equipment);
    void moveToGoalPos(Information* equipment);
    // The bot's route is a move to the same points, within 10 m each.
    bool isSameWaypointList(WaypointList* current, WaypointList* other);
    // Sends the bot out of a danger zone; false when there is nowhere to go.
    bool avoidArtillery(const Vec3& position, double since, int bySide);
    void switchToThirdWeapon(Information* equipment);

    // [INFERRED] carrier name. The two fields are attested by setCurrentRequestAction
    // its first parameter is the RequestAction enum, its second unsigned int.
    struct RequestState
    {
        int m_action;
        unsigned int m_handle;
    };

    // Offsets below are ELF. PE matches through +0x20, then adds four bytes of double alignment.
    /* +0x00 */ int m_squadIndex;
    /* +0x04 */ int m_memberIndex;
    /* +0x08 */ bool m_isSquadLeader;
    /* +0x0C */ int m_playerId;
    /* +0x10 */ IAIPlayer* m_player;
    /* +0x14 */ Bot* m_bot;
    /* +0x18 */ SquadCoordinator* m_squad;
    /* +0x1C */ int m_side;                  // zero-based, unlike the engine player's side
    /* +0x20 */ float m_estimatedArrival;    // getEstimatedArrival
    /* +0x24 */ double m_time24;             // last path reconsideration time; initialized to zero
    /* +0x2C */ float m_maxSpeed;            // getMaxSpeed
    /* +0x30 */ bool m_flag30;               // isAtFormationSlot
    /* +0x31 */ bool m_havePath;
    /* +0x34 */ RequestState* m_request;
    /* +0x38 */ int m_int38;                 // [UNRESOLVED]
    /* +0x3C */ Vec3 m_position;
    /* +0x48 */ bool m_positionValid;
    /* +0x49 */ bool m_isIdle;               // get/setIsIdle
    /* +0x4A */ bool m_flag4A;               // setHavePath writes the inverse of its argument
    /* +0x4B */ bool m_flag4B;               // isAvoidingArtillery
    /* +0x4C */ float m_formationDistanceSq; // [INFERRED] name, starts at 900.0
    /* +0x50 */ bool m_moved;
    /* +0x54 */ Vec3 m_vector54;             // [UNRESOLVED]
    /* +0x60 */ Vec3 m_vector60;             // [UNRESOLVED]
};

#if defined(_MSC_VER)
static_assert(sizeof(void*) != 4 || sizeof(SquadMember) == 112, "SquadMember PE x86 layout");
#endif

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SQUADMEMBER_H
