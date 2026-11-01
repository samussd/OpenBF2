// Owns squad coordinators and side-specific squad state.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_SQUADMANAGER_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_SQUADMANAGER_H

#include <list>
#include <map>
#include <vector>
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class AIStrategicArea;
class SquadCoordinator;   // (`operator new(0xE0)`); held only by pointer here
class SquadMember;
class IAIPlayer;
// Attested enum name in both squad-strength ELF manglings. Enumerator names are not recovered.
enum StrategicStrengthType : int;

class SquadManager
{
public:
    SquadManager();

    // Direct AIMain scheduling call, double by value; neither image materializes a result.
    void update(double time);
    void updateArtilleryStatus(double time);
    void updateCommanderMessages();
    void updateHumanPlayers(double time);   // bounded
    void setPlayerOnValidPosition(IAIPlayer* player, bool valid, double time);

    // player id then one-based side. All two callers in
    // each image discard the result; neither body materializes one independently of its last
    // updateSquadLeader call. void is the recovered no-result contract, not decompiler EAX.
    void addSquadMember(int playerId, int side);
    // Both take the player out of its squad on the engine's side too; the second one deletes
    // the member record.
    void removeSquadMember(int playerId, int side);
    void destroySquadMember(int playerId, int side);

    // Engine notifications use one-based side and squad ids. These bodies materialize no
    // result; the engine/courier callers discard it on both ports.
    void newSquadLeader(int side, int squadId);
    void playerJoinsSquad(int playerId, int squadId, int side);
    void playerRemovedFromSquad(int playerId, int squadId, int side);
    SquadMember* getSquadMember(int playerId, int side);
    SquadCoordinator* getSquadFromPlayer(int playerId, int side);
    SquadCoordinator* getSquadFromId(int squadId, int side);      // one-based, unchecked
    // An order that reached a player's squad: type 3 is a repair, the others its mission.
    void setSquadReceivedOrder(int type, int playerId, bool fromCommander);
    // The same for the player occupying the unit behind an information handle, if any.
    SquadMember* getSquadMember(unsigned int handle, int side);
    SquadCoordinator* getSquadFromPlayer(unsigned int handle, int side);
    // Whether a member of any of the side's squads already answers the request.
    bool isAnySquadMemberRespondingToRequest(unsigned int handle, int action, int side);
    // A member's factor to answer a request: 1 when it already makes this request, else its
    // squad's SquadCoordinator::requestResponseModifier; without a squad 0.
    float requestResponseModifier(SquadMember* member, unsigned int handle, int action, int side);
    int getSquadStrengths(int side, int areaIndex, StrategicStrengthType type);
    // The nearest available squad with members and strength, skipping one requested to the
    // area unless a bot leads it. strength receives twice its attack or defence strength.
    SquadCoordinator* getClosestAvailableSquad(AIStrategicArea* area, const Vec3& position,
                                               int side, bool attack, int& strength);
    float getDistanceToClosestSquad(const Vec3& position, int side);

    // Round restart: every squad loses its area, the queues empty, and both cursors rewind.
    void reset();
    void unAssignSquads(int side);

    // Artillery. Sides are one-based and unchecked.
    float getArtillerySafeDistance();
    // Where the side's artillery falls.
    Vec3 getArtilleryTargetPos(int side) const;
    // The last place a member's side fled to from it, one for soldiers and one for vehicles.
    bool haveValidArtilleryFleePos(SquadMember* member, Vec3& position);
    void setValidArtilleryFleePos(SquadMember* member, const Vec3& position);
    bool friendliesCloseTo(Vec3 position, float distance, int side);
    // Whether a human player is on the side.
    bool haveHumanPlayerOnOwnSide(int side);
    bool haveCampingHumans(int side, Vec2& position);
    void incomingArtillery(Vec3 position, int side);
    // Whether the position is within the artillery safe distance of a strike: the side's own
    // as soon as it is called, the other side's once its delay has run. timeSince is the time
    // since that strike was called (-1 for none) and bySide the side that called it (0 for none).
    bool isInArtilleryDangerZone(Vec3 position, int side, double& timeSince, int& bySide);
    // Frees the side's squads assigned to the area, except those already closing on it; returns
    // how many were freed.
    int unAssignSquads(int areaIndex, int side);

    // Clears messages and position records by erasing each container node individually.
    void clearCommanderMessages();
    // Queues a message for the delay from now, unless the squad of the side already has one.
    void enqueueCommanderMessage(int side, int squad, int type, double delay);
    void clearPlayerValidPositions();

    static SquadManager* m_instance;

    // Queued commander message. Field and enumerator names are inferred.
    // Native dueTime offset/record size: ELF +0xC/20 bytes, PE +0x10/24 bytes.
    enum CommanderMessageType : int;
    class CommanderMessage
    {
    public:
        CommanderMessageType m_type;
        int m_squadIndex;
        int m_sideIndex;
        double m_dueTime;
    };

    // Three Vec3 values, not containers: incomingArtillery
    // and setValidArtilleryFleePos store their scalar components.
    // [INFERRED] field names. ELF offsets below; PE offsets are 0,8,16,32,40,41,44,56.
    // Only the three positions are zeroed here. The manager tail clears m_b0; both doubles
    // and the two validity flags remain unwritten by retail construction.
    class SideBlock
    {
    public:
        /*  +0 */ bool m_b0;
        /*  +4 */ double m_artilleryStartTime;
        /* +12 */ Vec3 m_artilleryPosition{};
        /* +24 */ double m_artilleryDelay;
        /* +32 */ bool m_validFlee0;
        /* +33 */ bool m_validFlee1;
        /* +36 */ Vec3 m_fleePos0{};
        /* +48 */ Vec3 m_fleePos1{};
    };

private:
    // no vtable. The constructor stores no vptr on any path, so this class has no virtuals at
    // all - checked before writing, because a missing vptr shifts every offset below by four.
    /*   0 */ std::vector<SquadCoordinator*> m_coordinators[2]; // ELF stride 12 / PE stride 16
    /*  24 */ int  m_squadCursor;         // = 0; [INFERRED] name, update uses unsigned comparison
    /*  28 */ int  m_sideCursor;          // = 1; [INFERRED] name, update alternates zero/nonzero
    // Per-side state blocks, each 60 bytes in the original layout. Construction clears
    // the leading flag and the same nine words in each block.
    /*  32 */ SideBlock m_sideBlock0;     // +32..+91
    /*  92 */ SideBlock m_sideBlock1;     // +92..+151
    /* 152 */ float  m_float152;          // = 40.0f (bit pattern )
    /* 156 */ double m_double156;         // = 25.0
    // with a bare `operator delete` one at a time.
    /* 164 */ std::list<CommanderMessage*> m_commanderMessages;
    // Maps player ids to their first invalid-position timestamp; a valid position removes the entry.
    /* 168 */ std::map<int, double> m_playerValidPositions;
    // The original ELF map includes its empty comparator at +176 and ends at +180.
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SQUADMANAGER_H
