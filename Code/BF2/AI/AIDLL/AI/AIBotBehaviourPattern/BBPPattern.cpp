#include "BBPPattern.h"

#include "../AIBot/BotMain.h"                  // Bot::getBotId
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

BBPPattern::BBPPattern()
{
    const int numberOfBots = getNumberOfBots();
    m_initiated = new bool[numberOfBots];
    for (int i = 0; i < numberOfBots; ++i)
        m_initiated[i] = false;
}

BBPPattern::~BBPPattern()
{
    delete[] m_initiated;
}

bool BBPPattern::isInitiated(Bot* bot)
{
    return m_initiated[bot->getBotId()];
}

void BBPPattern::unInit(Bot* bot)
{
    m_initiated[bot->getBotId()] = false;
}

void BBPPattern::forceInitiated(Bot* bot)
{
    m_initiated[bot->getBotId()] = true;
}

int BBPPattern::getNumberOfBots()
{
    return static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings)->getMaxNBots();
}

} // namespace ai
} // namespace hfe
} // namespace dice
