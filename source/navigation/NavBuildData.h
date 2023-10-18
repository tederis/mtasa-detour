#pragma once

#include <vector>

#include "../utils/MathUtils.h"

class rcContext;

struct dtTileCacheContourSet;
struct dtTileCachePolyMesh;
struct dtTileCacheAlloc;
struct rcCompactHeightfield;
struct rcContourSet;
struct rcHeightfield;
struct rcHeightfieldLayerSet;
struct rcPolyMesh;
struct rcPolyMeshDetail;

namespace WorldAssistant
{

// Navigation area stub.
struct NavAreaStub
{
    // Area bounding box.
    BoundingBox bounds_;
    // Area ID.
    unsigned char areaID_{};
};

struct NavBuildData
{
    // Constructor.
    NavBuildData();
    // Destructor.
    virtual ~NavBuildData();

    // Vertices from geometries.
    std::vector<Vector3F> vertices_;
    // Triangle indices from geometries.
    std::vector<std::int32_t> indices_;
    // Recast context.
    rcContext* ctx_;
    // Recast heightfield.
    rcHeightfield* heightField_;
    // Recast compact heightfield.
    rcCompactHeightfield* compactHeightField_;
    // Pretransformed navigation areas, no correlation to the geometry above.
    std::vector<NavAreaStub> navAreas_;
};

struct DynamicNavBuildData : public NavBuildData
{
    // Constructor.
    explicit DynamicNavBuildData(dtTileCacheAlloc* allocator);
    // Destructor.
    ~DynamicNavBuildData() override;

    // TileCache specific recast contour set.
    dtTileCacheContourSet* contourSet_;
    // TileCache specific recast poly mesh.
    dtTileCachePolyMesh* polyMesh_;
    // Recast heightfield layer set.
    rcHeightfieldLayerSet* heightFieldLayers_;
    // Allocator from DynamicNavigationMesh instance.
    dtTileCacheAlloc* alloc_;
};

}