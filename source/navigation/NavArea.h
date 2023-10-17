#pragma once

#include "../utils/MathUtils.h"

namespace WorldAssistant
{

class NavArea
{
public:
	bool IsEnabled() const { return enabled_; }

	// Get the area id for this volume.
    unsigned GetAreaID() const { return (unsigned)areaID_; }

	// Set the area id for this volume.
    void SetAreaID(unsigned newID);

	// Get the bounding box of this navigation area, in local space.
	const BoundingBox& GetBoundingBox() const { return bounds_; }

	 // Set the bounding box of this area, in local space.
	void SetBoundingBox(const BoundingBox& bounds);

private:
	bool enabled_{};

	// Bounds of area to mark.
	BoundingBox bounds_;

	// Area id to assign to the marked area.
	unsigned char areaID_{};
};

}