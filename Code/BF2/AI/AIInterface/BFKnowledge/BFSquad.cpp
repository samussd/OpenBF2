// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFSquad operations.

#include "BFSquad.h"
#include "AIPlayer.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../../../Game/GameLogic/SquadManager/ISquadManager.h"
#include "../../../Game/GameLogic/SquadManager/SquadOrder.h"
#include "../../../Game/GameEvents/RequestEvent.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../IO/Network/INetworkable.h"
#include "../../../IO/Network/NetworkManager.h"
#include "IAIObject.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods

namespace dice {
namespace hfe {

extern ClassManager* classManager;
extern IBase* squadManager;

namespace ai {

namespace {

// The target travels by network id. Attack orders are type 5, move 4 and defend 6.
SquadOrder makeOrder(const Vec3& position, IAIObject* target, int type)
{
    SquadOrder order = {};
    order.orderId = type;
    order.points[0] = position;
    if (target)
    {
        io::INetworkable* const networkable = target->getObject()->getNetworkable();
        if (networkable)
            order.targetNetId = networkable->m_networkableId;
    }
    return order;
}

// What a bot's order or request looks like to the server's own request handling.
void sendRequest(int squad, IAIPlayer* player, int type)
{
    RequestEvent* const event = new RequestEvent(player->getSide(), squad,
                                                 static_cast<RequestEvent::RequestTypes>(type));
    event->executeServer(player->getId());
    event->release();
}

void answerCommanderOrder(int squad, IAIPlayer* player, int type, bool accepted)
{
    static_cast<ISquadManager*>(squadManager)->setIsAccepted(
        player->getSide(), squad, accepted, static_cast<OrderType>(1),
        static_cast<SquadOrderType>(type), false);
    if (player->getIsAIPlayer())
        sendRequest(squad, player, accepted ? 2 : 3);
}

} // anonymous namespace

// First wins in, exact match out.
IBFSquad::IBFSquad()
{
    if (!CommonSingletonServer::m_iBFSquad)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->registerSimpleSingleton(std::string("IBFSquad"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IBFSquad::~IBFSquad()
{
    if (CommonSingletonServer::m_iBFSquad == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IBFSquad"));
        BFSingletonServer::updateStaticMethods();
    }
}

BFSquad::BFSquad()
{
}

BFSquad::~BFSquad()
{
}

int BFSquad::getNextEmptySquad(int team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFSquad::getNextEmptySquad");
}

// Preserve id-before-side virtual calls.
bool BFSquad::joinSquad(int squad, IAIPlayer* player)
{
    const int playerId = player->getId();
    const int side = player->getSide();
    return static_cast<ISquadManager*>(squadManager)->addToSquad(side, squad, playerId);
}

// Preserve id-before-side virtual calls. Never a kick.
void BFSquad::exitSquad(int squad, IAIPlayer* player)
{
    const int playerId = player->getId();
    const int side = player->getSide();
    static_cast<ISquadManager*>(squadManager)->removeFromSquad(side, squad, playerId, false);
}

bool BFSquad::isSquadLeader(int squad, IAIPlayer* player)
{
    // [PLATFORM] ELF 0x84109C0 reads id, side, leader; PE 0x742110 reads side, leader, id.
#if defined(_MSC_VER)
    const int side = player->getSide();
    const int leader = static_cast<ISquadManager*>(squadManager)->getSquadLeader(side, squad);
    const int playerId = player->getId();
#else
    const int playerId = player->getId();
    const int side = player->getSide();
    const int leader = static_cast<ISquadManager*>(squadManager)->getSquadLeader(side, squad);
#endif
    return playerId == leader;
}

int BFSquad::assignAsSquadLeader(int squad, IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFSquad::assignAsSquadLeader");
}

int BFSquad::retireAsSquadLeader(int squad, IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFSquad::retireAsSquadLeader");
}

int BFSquad::issueSquadAttackOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    SquadOrder order = makeOrder(position, target, 5);
    static_cast<ISquadManager*>(squadManager)->issueSquadOrder(b, a, order, false);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

int BFSquad::issueSquadMoveOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    SquadOrder order = makeOrder(position, target, 4);
    static_cast<ISquadManager*>(squadManager)->issueSquadOrder(b, a, order, false);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

int BFSquad::issueSquadDefendOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    SquadOrder order = makeOrder(position, target, 6);
    static_cast<ISquadManager*>(squadManager)->issueSquadOrder(b, a, order, false);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

// 5 bytes - a bare ret. Empty, like its decline twin.
void BFSquad::acceptSquadOrder(int squad, IAIPlayer* player)
{

}

// ELF, - empty.
void BFSquad::declineSquadOrder(int squad, IAIPlayer* player)
{

}

int BFSquad::acceptCommanderAttackOrder(int squad, IAIPlayer* player)
{
    answerCommanderOrder(squad, player, 5, true);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

int BFSquad::acceptCommanderDefendOrder(int squad, IAIPlayer* player)
{
    answerCommanderOrder(squad, player, 6, true);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

int BFSquad::acceptCommanderMoveOrder(int squad, IAIPlayer* player)
{
    answerCommanderOrder(squad, player, 4, true);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

int BFSquad::declineCommanderAttackOrder(int squad, IAIPlayer* player)
{
    answerCommanderOrder(squad, player, 5, false);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

int BFSquad::declineCommanderDefendOrder(int squad, IAIPlayer* player)
{
    answerCommanderOrder(squad, player, 6, false);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

int BFSquad::declineCommanderMoveOrder(int squad, IAIPlayer* player)
{
    answerCommanderOrder(squad, player, 4, false);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

int BFSquad::requestNewCommanderOrder(int squad, IAIPlayer* player)
{
    if (player->getIsAIPlayer())
        sendRequest(squad, player, 1);
    return 0;   // [CHOSEN, not recovered] no caller reads it
}

world::IObject* BFSquad::getCommanderOrder(int a, int b, Vec3& position, IAIObject* target)
{
    const SquadOrder* const order =
        static_cast<ISquadManager*>(squadManager)->getSquadInfo(a, b)->getCommanderOrders();
    position = order->points[0];
    return io::networkManager->getObject(order->targetNetId);
}

world::IObject* BFSquad::getSquadOrder(int a, int b, Vec3& position, IAIObject* target)
{
    const SquadOrder* const order =
        static_cast<ISquadManager*>(squadManager)->getSquadInfo(a, b)->getSquadOrders();
    position = order->points[0];
    return io::networkManager->getObject(order->targetNetId);
}

} // namespace ai
} // namespace hfe
} // namespace dice
