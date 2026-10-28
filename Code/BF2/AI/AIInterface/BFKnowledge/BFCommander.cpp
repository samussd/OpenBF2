// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFCommander operations.

#include "BFCommander.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods
#include "../../../Core/EventManager.h"                       // fireArtillery's request event
#include "../../../Game/GameEvents/ArtilleryEvent.h"
#include "../../../Game/GameLogic/SquadManager/ISquadManager.h"
#include "../../../Game/IEventListener.h"                     // ECGameLogic
#include "../../../Game/Objects/Game/ArtilleryHandler.h"      // the per-team artillery handlers

namespace dice {
namespace hfe {

extern ClassManager* classManager;
extern IBase* squadManager;

namespace ai {

// First wins in, exact match out.
IBFCommander::IBFCommander()
{
    if (!CommonSingletonServer::m_iBFCommander)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->registerSimpleSingleton(std::string("IBFCommander"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IBFCommander::~IBFCommander()
{
    if (CommonSingletonServer::m_iBFCommander == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IBFCommander"));
        BFSingletonServer::updateStaticMethods();
    }
}

BFCommander::BFCommander()
{
}

BFCommander::~BFCommander()
{
}

int BFCommander::applyAsCommander(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::applyAsCommander");
}

int BFCommander::retireAsCommander(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::retireAsCommander");
}

int BFCommander::giveAttackOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::giveAttackOrder");
}

int BFCommander::giveMoveOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::giveMoveOrder");
}

int BFCommander::giveDefendOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::giveDefendOrder");
}

void BFCommander::sendAttackMessage(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::sendAttackMessage");
}

void BFCommander::sendDefendMessage(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::sendDefendMessage");
}

// Requests a strike as the team's commander would, through a server-local artillery event; a
// squad other than -1 also has its order marked accepted.
void BFCommander::fireArtillery(int team, int squad, Vec2 position)
{
    ArtilleryEvent* const event = new ArtilleryEvent();
    event->m_target = position;
    event->m_artilleryType = ArtilleryEvent::ArtilleryTypeRequestFire;
    event->m_team = static_cast<unsigned char>(team);
    eventManager->postEventNextUpdate(ECGameLogic, 15, event);   // ELF +0x1C / PE +0x18
    if (squad != -1)
    {
        static_cast<ISquadManager*>(squadManager)->setIsAccepted(
            team, squad, false, static_cast<OrderType>(2), static_cast<SquadOrderType>(0), false);
    }
}

int BFCommander::sendUav(int a, int b, Vec2 position)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::sendUav");
}

int BFCommander::dropSupply(int a, int b, Vec2 position)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::dropSupply");
}

int BFCommander::dropVehicle(int a, int b, Vec2 position)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::dropVehicle");
}

int BFCommander::denyArtilleryRequest(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::denyArtilleryRequest");
}

// Whether the level gave the team any artillery piece, destroyed or not. The team is unchecked.
bool BFCommander::gameHasArtillery(int team)
{
    return world::artilleryManager->getHandler(static_cast<unsigned char>(team))
        ->hasArtilleryPieces();
}

// Whether any of the team's pieces still stands. The team is unchecked.
bool BFCommander::anyArtilleryPiecesLeft(int team)
{
    return world::artilleryManager->getHandler(static_cast<unsigned char>(team))
        ->haveAnyArtillery();
}

// dice's capital A. Frozen - it is the mangled name.
int BFCommander::ArtilleryUnavailable(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::ArtilleryUnavailable");
}

int BFCommander::denySupplyRequest(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::denySupplyRequest");
}

int BFCommander::denyVehicleDropRequest(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::denyVehicleDropRequest");
}

int BFCommander::denyUavRequest(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::denyUavRequest");
}

} // namespace ai
} // namespace hfe
} // namespace dice
