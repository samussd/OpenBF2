
#include "AIEventListener.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "../../../Core/EventManager.h"
#include "../../../Core/Debug.h"                     // the line-166 and line-494 diagnostics below
#include "../../../Game/Objects/IPlayer.h"          // world::IPlayer::getId / getIsAIPlayer
#include "../SingletonServer.h"                     // BFSingletonServer::m_iaiEventCourier
#include "../BFKnowledge/BFGame.h"                  // IAIGame::setIsPrecaching / isPlaying
#include "../BFKnowledge/AIPlayerFactory.h"         // IAIPlayerFactory, isOccupiedObject
#include "../BFKnowledge/IAIObject.h"               // the vehicle arms' object slots
#include "../BFKnowledge/IAIObjectPlugIn.h"         // IAIObjectUnit / IAIObjectPhysical
#include "../../AIDLL/AI/AIKnowledge/BFInformationCourier.h"  // IAIInformationCourier
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../AIDLL/AI/Main/AIEventCourier.h"     // IAIEventCourier - 55 named slots
#include "../../AIDLL/AI/Main/AISettingsInterfaces.h"  // IAISettings::getDraw
#include "../../AIDLL/AI/Main/AIMineManager.h"      // IAIMineManager::mineRemoved
#include "../../../Game/GameServer/ServerSettings.h"  // the team-ratio inversion below
#include "../../../Scene/Object/PlayerManager.h"      // the three lookups it needs
#include "../../AIDLL/AI/DrawPrimitives/DPManager.h"// IDPManager::drawAll / nodrawUpdate

namespace dice {
namespace hfe {

extern EventManager* eventManager;

namespace ai {

// The vptr and two bytes; nothing else.
AIEventListener::AIEventListener()
    : m_initialised(true)
    , m_unknown05(false)
{
}

// Unregisters category-3 events 0..23, then handlers 2, 3, 1, 0, 5 in initialization
// order. Does not guard against an uninitialized listener.
AIEventListener::~AIEventListener()
{
    for (int i = 0; i <= 23; ++i)
        eventManager->unregisterEvent(static_cast<EventCategory>(3), i);

    eventManager->unregisterEventHandler(static_cast<EventCategory>(2), this);
    eventManager->unregisterEventHandler(static_cast<EventCategory>(3), this);
    eventManager->unregisterEventHandler(static_cast<EventCategory>(1), this);
    eventManager->unregisterEventHandler(static_cast<EventCategory>(0), this);
    eventManager->unregisterEventHandler(static_cast<EventCategory>(5), this);
}

// Marks the listener initialized and registers handlers 2, 3, 1, 0, 5 at priority zero
// even when it was already initialized.
void AIEventListener::init()
{
    m_initialised = true;

    eventManager->registerEventHandler(static_cast<EventCategory>(2), this, 0);
    eventManager->registerEventHandler(static_cast<EventCategory>(3), this, 0);
    eventManager->registerEventHandler(static_cast<EventCategory>(1), this, 0);
    eventManager->registerEventHandler(static_cast<EventCategory>(0), this, 0);
    eventManager->registerEventHandler(static_cast<EventCategory>(5), this, 0);

    registerEvents();
}

// Registers category-3 event ids 0..23 under the same literal name, "ECAIi".
// Constructs each name inside the loop and does not require a listener instance.
void AIEventListener::registerEvents()
{
    for (int i = 0; i <= 23; ++i)
        eventManager->registerEvent(static_cast<EventCategory>(3), i, std::string("ECAIi"));
}

// Routes engine events to the AI couriers. Unsupported event paths retain named frontier stops.
// [RETIRE-WHEN: aidll-01-adapters] Remove when the corresponding implementation is complete and validated.

namespace {

// [FRONTIER] [TEMP] [CHOSEN, not recovered] - one per unwritten arm, each naming its own
// (category, id) so the next person knows exactly which dispatch entry is wanted rather than only
// that `handleEvent` was reached. [RETIRE-WHEN: aidll-01-adapters]
void unwrittenArm(const char* what, int category, unsigned int id)
{
    std::fprintf(stderr,
                 "[FRONTIER] dice::hfe::ai::AIEventListener::handleEvent - %s" "\n"
                 "           category=%d id=%u" "\n",
                 what, category, id);
    std::fflush(stderr);
    std::abort();
}

// id 1, as PlayerControlObject::enter and Soldier post it: 20 bytes. A soldier sends itself as
// the seat and no body.
struct EnterVehicleNote
{
    IAIObject* seat;
    IAIObject* body;
    int        team;
    int        playerId;
    bool       specialContainer;    // ladder, parachute, zipline or grappling-hook rope
};

// id 2: the same fields with the two objects swapped. A soldier sends itself as the body.
struct ExitVehicleNote
{
    IAIObject* body;
    IAIObject* seat;
    int        team;
    int        playerId;
    bool       specialContainer;
};

// id 0, from ServerGameLogic.
struct PlayerKilledNote
{
    int        team;
    int        playerId;
    IAIObject* vehicle;
};

// ids 6 and 7, from Soldier.
struct SoldierNote
{
    IAIObject* soldier;
    int        playerId;
};

// Returns a seat's armament to the seat, on the object and in both sides' information.
// Retail repeats this tail in ids 0, 2 and 14.
void returnBorrowedArmament(IAIObject* seat)
{
    IAIInformationCourier* const information =
        static_cast<IAIInformationCourier*>(CommonSingletonServer::m_iaiInformationCourier);
    seat->resetArmament();
    information->resetArmament(seat->getHandle(1));
    information->resetArmament(seat->getHandle(2));
}

} // anonymous namespace

void AIEventListener::handleEvent(EventCategory category,
                                  unsigned int  id,
                                  EventNode*    /*node*/,
                                  void*         userData)
{
    IAIEventCourier* const courier = BFSingletonServer::m_iaiEventCourier;

    switch ((int)category)
    {
    // category 0
    case 0:
        if (id == 1)
        {
            // The payload's first word is the state: 1 starts a round, 2 ends one.
            const int state = *static_cast<const int*>(userData);
            if (state == 1)
                courier->eventGameStart();
            else if (state == 2)
                courier->event_roundEnd();
        }
        else if (id == 6)
        {
            // the only null check on the courier in the whole function. Retail tests it here and
            // nowhere else; reproduced rather than made consistent.
            if (courier)
                courier->event_worldReset();
        }
        else if (id == 8)
        {
// Updates the AI team ratio when the only human player switches sides.
            const int* const p = static_cast<const int*>(userData);

            const int playerId = p[0];

            if (world::playerManager->getPlayerFromId(playerId)
                    == world::playerManager->getLocalHumanPlayer()
                && world::playerManager->getNumberOfHumanPlayers() == 1)
            {
                serverSettings->m_teamRatioPercent =
                    static_cast<float>(1.0f / (serverSettings->getTeamRatioPercent() * 0.0099999998f)
                                       * 100.0f);
            }

            courier->event_playerSwitchSides(p[1], playerId);
            return;
        }
        return;

    // category 1: player lifecycle
    case 1:
        {
            world::IPlayer* const player = static_cast<world::IPlayer*>(userData);
            if (id == 2)
            {
                // After level unloading begins, reports disconnects only for AI players.
                if (!m_unknown05 || player->getIsAIPlayer())
                    courier->eventPlayerDisconnect(player->getId());
            }
            else if (id == 4)
            {
                courier->eventPlayerCreated(player->getIsAIPlayer());
            }
            else if (id == 6)
            {
                courier->eventPlayerSpawned(player->getId());
            }
            else if (id == 10)
            {
                courier->event_freeCameraFire(*static_cast<const int*>(userData) != 0);
            }
        }
        return;

    // category 3: the AI's own twenty-four events
    case 3:
        switch (id)
        {
        case 4u:
            // Draws debug primitives when enabled; otherwise only ages them.
            if (static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings)->getDraw())
            {
                courier->eventDrawDebug();
                static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager)->drawAll();
            }
            else
            {
                static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager)->nodrawUpdate();
            }
            return;

        case 8u:
            {
                // The payload is (target, orderType); 4/5/6 select move/attack/defend.
                const int* const p = static_cast<const int*>(userData);
                if (p[1] == 5)
                    courier->event_recievedAttackCommanderOrder(p[0]);
                else if (p[1] == 6)
                    courier->event_recievedDefendCommanderOrder(p[0]);
                else if (p[1] == 4)
                    courier->event_recievedMoveCommanderOrder(p[0]);
            }
            return;

        case 9u:
            {
                // The same shape one order-type wider - squad orders add 7, repair.
                const int* const p = static_cast<const int*>(userData);
                if (p[1] == 5)
                    courier->event_recievedAttackSquadOrder(p[0]);
                else if (p[1] == 6)
                    courier->event_recievedDefendSquadOrder(p[0]);
                else if (p[1] == 7)
                    courier->event_receivedRepairSquadOrder(p[0]);
                else if (p[1] == 4)
                    courier->event_recievedMoveSquadOrder(p[0]);
            }
            return;

        case 0xBu:
            {
                const int* const p = static_cast<const int*>(userData);
                courier->event_botTeamKilled(p[0], p[1]);
            }
            return;

        case 0xCu:
            static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
                ->create(static_cast<world::IPlayer*>(userData));
            return;

        case 0xDu:
            {
                IAIPlayerFactory* const factory =
                    static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory);
                IAIPlayer* const aiPlayer = factory->getPlayerFromId(
                    static_cast<world::IPlayer*>(userData)->getId());
                if (aiPlayer)
                    factory->destroy(aiPlayer);
            }
            return;

        case 0x10u:
            {
                // retail copies five payload words to the stack here and uses two of them. The
                // other three are dead stores the compiler kept. Only the two live ones are written.
                const int* const p = static_cast<const int*>(userData);
                courier->event_requestNewOrders(p[1], p[0]);
            }
            return;

        case 0x15u:
            {
                // two subsystems are told, in this order: the mine manager first so its polygon
                // sets are current, then the AI proper.
                const unsigned int mineId = *static_cast<const unsigned int*>(userData);
                static_cast<IAIMineManager*>(CommonSingletonServer::m_iAIMineManager)
                    ->mineRemoved(mineId);
                courier->event_mineRemoved(mineId);
            }
            return;

        case 0x16u:
            {
                // The payload is an object pointer followed by a team byte at offset 4.
                const unsigned char* const bytes = static_cast<const unsigned char*>(userData);
                courier->event_enemySpotted(*static_cast<IAIObject* const*>(userData), bytes[4]);
            }
            return;

        case 0x17u:
            courier->event_isRepaired(*static_cast<const int*>(userData));
            return;

        case 0u:
            {
                const PlayerKilledNote* const note = static_cast<const PlayerKilledNote*>(userData);
                courier->eventPlayerKilled(note->playerId, note->team);

                // A seat that borrowed its occupant's weapons gets its own back.
                IAIObject* const vehicle = note->vehicle;
                if (!vehicle || !vehicle->isUnit())
                    return;
                if (!vehicle->getUnit()->mustBorrowArmament(vehicle))
                    return;
                returnBorrowedArmament(vehicle);
            }
            return;

        case 1u:
            {
                const EnterVehicleNote* const note = static_cast<const EnterVehicleNote*>(userData);
                IAIObject* const seat = note->seat;
                IAIObject* primary;
                if (!seat)
                {
                    if (note->specialContainer)
                        return;
                    Debug(DebugTypeError, __FILE__, 166, "")
                        << "You have tried to enter a vehicle that is not prepared for use with AI."
                           " Game will crash.";
                    // Falls through to seat->setSide on the null seat, as the message says.
                    primary = 0;
                }
                else
                {
                    primary = seat->getPrimaryObject();
                }

                IAIObject* const body = note->body;
                seat->setSide(note->team);
                if (primary)
                {
                    if (IAIObjectPhysical* const physical = primary->getPhysical())
                        physical->enablePhysics(note->team);
                }

                // A seat that fights with its occupant's weapons borrows them, on the object and
                // in both sides' information.
                if (body && seat->getUnit() && seat->getUnit()->mustBorrowArmament(seat))
                {
                    IAIInformationCourier* const information =
                        static_cast<IAIInformationCourier*>(
                            CommonSingletonServer::m_iaiInformationCourier);
                    seat->borrowArmament(body);
                    information->borrowArmament(body->getHandle(1), seat->getHandle(1));
                    information->borrowArmament(body->getHandle(2), seat->getHandle(2));
                }

                if (body)
                    body->removeFromGrid();
                courier->eventPlayerEnterVehicle(note->playerId, seat, body);
            }
            return;

        case 2u:
            {
                const ExitVehicleNote* const note = static_cast<const ExitVehicleNote*>(userData);
                if (note->specialContainer)
                    return;

                IAIObject* const seat = note->seat;
                IAIObject* const body = note->body;
                if (seat)
                {
                    if (seat->getUnit() && seat->getUnit()->mustBorrowArmament(seat))
                        returnBorrowedArmament(seat);

                    // The seat stays on its side while anyone else is still aboard.
                    if (!isOccupiedObject(seat, seat))
                        seat->setSide(0);
                }
                if (body)
                    body->reInsertInGrid();
            }
            return;

        case 3u:
            {
                // ServerGameLogic's two-word exit note: the player id, then the vehicle's object.
                const int* const p = static_cast<const int*>(userData);
                courier->eventPlayerExitVehicle(reinterpret_cast<IAIObject*>(p[1]), p[0]);
            }
            return;

        case 5u:
            static_cast<IAIObject*>(userData)->update();
            return;

        case 6u:
            {
                const SoldierNote* const note = static_cast<const SoldierNote*>(userData);
                courier->eventManDown(note->playerId, note->soldier);
            }
            return;

        case 7u:
            {
                const SoldierNote* const note = static_cast<const SoldierNote*>(userData);
                note->soldier->update();
                courier->eventRevived(note->playerId, note->soldier);
            }
            return;

        case 0xAu:
            unwrittenArm("category 3 id 10 - radio message (needs BFConsole::getAIMessageSettings,"
                         " itself a frontier)", (int)category, id);
            return;
        case 0xEu:
            {
                IAIObject* const vehicle = static_cast<IAIObject*>(userData);
                if (!vehicle || !vehicle->getUnit())
                    return;
                if (!vehicle->getUnit()->mustBorrowArmament(vehicle))
                    return;
                returnBorrowedArmament(vehicle);
            }
            return;

        // [UNRESOLVED] Retail reads a 0x44-byte payload here; RequestEvent posts a 0x30-byte one.
        case 0xFu:
            unwrittenArm("category 3 id 15 - commander request (artillery/uav/supply/vehicle drop),"
                         " a 68-byte payload struct", (int)category, id);
            return;

        case 0x11u:
            {
                // ArmorGLComp's note: the first damaging player's id, then the bridge's object.
                if (!static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->isPlaying())
                    return;
                const int* const p = static_cast<const int*>(userData);
                IAIObject* const bridge = reinterpret_cast<IAIObject*>(p[1]);
                courier->event_pathfindingMapIdDestroyed(bridge->getPhysical()->getMapPosId(), 0);
                courier->event_bridgeDestroyed(bridge, p[0]);
            }
            return;

        case 0x12u:
            {
                IAIObject* const bridge = static_cast<IAIObject*>(userData);
                courier->event_pathfindingMapIdRestored(bridge->getPhysical()->getMapPosId());
                courier->event_bridgeRepaired(bridge);
            }
            return;

        case 0x13u:
            static_cast<IAIObject*>(userData)->reInsertInGrid();
            return;

        case 0x14u:
            static_cast<IAIObject*>(userData)->removeFromGrid();
            return;

        default:
            // Reports an unknown AI event with the original diagnostic line number.
            Debug(DebugTypeError, __FILE__, 494, "") << "Unknown and unhandled AIEvent: " << id;
            return;
        }

    // category 5: level load/unload
    case 5:
        if (id == 2)
        {
            if (CommonSingletonServer::m_iaiGame)
                static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->setIsPrecaching(true);
        }
        else if (id == 6)
        {
            // Permanently restricts disconnect reporting to bots for this listener's remaining lifetime.
            m_unknown05 = true;
        }
        else if (id == 7)
        {
            if (CommonSingletonServer::m_iaiGame)
                static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->setIsPrecaching(false);
            courier->event_levelLoaded();
        }
        return;

    // everything else, including the whole of category 2
    default:
        return;
    }
}
} // namespace ai
} // namespace hfe
} // namespace dice
