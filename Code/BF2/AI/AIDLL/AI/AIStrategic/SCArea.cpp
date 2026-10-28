// [INFERRED] Source placement.

#include "SCArea.h"

#include "../AIStrategicEntity/AIStrategicArea.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"

namespace dice {
namespace hfe {
namespace ai {

SCArea::SCArea(const std::string& name, const std::string& areaName, int areaIndex,
               StatusMap::Status status)
    : StrategyCondition(name, 0.0f, static_cast<Compare>(9), Friendly, Crisp,
                        static_cast<Conditional>(44))
    , m_areaIndex(areaIndex)
    , m_areaName(areaName)
    , m_status(status)
{
}

SCArea::~SCArea()
{
}

int SCArea::getType() const
{
    return 3;
}

float SCArea::compare(float*, float*, int side) const
{
    const AIStrategicObject* const object =
        AIStrategicObjectManager::getInstance()->getStrategicObject(m_areaIndex);
    if (object->getType() == 0 && object->getStatus(side) == m_status)
        return 1.0f;
    return -1.0f;
}

int SCArea::updateArea()
{
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    manager->setActive(m_areaName);
    const AIStrategicArea* const area = manager->getActiveArea();
    m_areaIndex = area ? area->getIndex() : -1;
    return m_areaIndex;
}

} // namespace ai
} // namespace hfe
} // namespace dice
