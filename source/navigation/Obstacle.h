#pragma once

#include <memory>

#include "../utils/MathUtils.h"

namespace WorldAssistant
{

class DynamicNavigationMesh;

class Obstacle
{
    friend class DynamicNavigationMesh;
public:

    const Vector3F& GetWorldPosition() const { return worldPosition_; }

    float GetRadius() const { return radius_; }

    float GetHeight() const { return height_; }

    bool IsEnabled() const { return enabled_; }

private:
    Vector3F worldPosition_;

    bool enabled_{ true };

     // Radius of this obstacle.
    float radius_{};
    // Height of this obstacle, extends 1/2 height below and 1/2 height above the owning node's position.
    float height_{};

	// Id received from tile cache.
    unsigned obstacleId_{};
    // Pointer to the navigation mesh we belong to.
    std::weak_ptr<DynamicNavigationMesh> ownerMesh_;
};

}