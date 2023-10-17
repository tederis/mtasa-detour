#pragma once

#include "../utils/MathUtils.h"

namespace WorldAssistant
{

class OffMeshConnection
{
public:
	bool IsEnabled() const { return enabled_; }

    const Vector3F& GetStartPosition() const { return startPosition_; }

    const Vector3F& GetEndPosition() const { return endPosition_; }

    // Return radius.

    float GetRadius() const { return radius_; }

    // Return whether is bidirectional.

    bool IsBidirectional() const { return bidirectional_; }

    // Return the user assigned mask.

    unsigned GetMask() const { return mask_; }

    // Return the user assigned area ID.

    unsigned GetAreaID() const { return areaId_; }

private:
	bool enabled_{ true };

    Vector3F startPosition_;

    Vector3F endPosition_;

	 // Radius.
    float radius_{1.0f};
    // Bidirectional flag.
    bool bidirectional_{true};
    // Flags mask to represent properties of this mesh.
    unsigned mask_{1};
    // Area id to be used for this off mesh connection's internal poly.
    unsigned areaId_{1};
};

}