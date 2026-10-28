// A condition on one strategic area's status for the evaluating side.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGIC_SCAREA_H
#define AI_AIDLL_AI_AISTRATEGIC_SCAREA_H

#include <string>

#include "StatusMap.h"
#include "StrategyCondition.h"

namespace dice {
namespace hfe {
namespace ai {

class SCArea : public StrategyCondition
{
public:
    // The base gets target 0, an unknown Compare and Conditional, Friendly and Crisp.
    SCArea(const std::string& name, const std::string& areaName, int areaIndex,
           StatusMap::Status status);
    virtual ~SCArea();

    virtual int getType() const;
    // 1.0 when the area is an area object (type 0) whose status for side is the wanted one,
    // else -1.0. The area index is unchecked: updateArea must have resolved it.
    virtual float compare(float* friendly, float* enemy, int side) const;

    // Resolves the area name through the strategic object manager's active-area selection, which
    // it leaves changed. Returns the index, -1 for an unknown area.
    int updateArea();

    /* +0x24 */ int m_areaIndex;
    /* +0x28 */ std::string m_areaName;
    /* +0x2C */ StatusMap::Status m_status;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGIC_SCAREA_H
