#include "../navigation/NavArea.h"

namespace WorldAssistant
{

void NavArea::SetAreaID(unsigned newID)
{
	areaID_ = newID;
}

void NavArea::SetBoundingBox(const BoundingBox& bounds)
{
	bounds_ = bounds;
}

}